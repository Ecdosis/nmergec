#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#include "link_node.h"
#include "mvd/pair.h"
#include "mvd/version.h"
#include "mvd/mvd.h"
#include "dyn_array.h"
#include "mvd/version.h"
#include "mvd/mvdfile.h"
#include "mvd/serialiser.h"
#include "mvd/group.h"
#include "utils.h"
#include "hashmap.h"

#define DEFAULT_VERSION_SIZE 32
#define DEFAULT_PAIRS_SIZE 4096
#define DEFAULT_ENCODING "UTF-8"
#define DEFAULT_SET_SIZE 32
	
struct MVD_struct
{
    dyn_array *versions;	// id = position in table+1
	dyn_array *pairs;
	char *description;
	char *encoding;
    /** #bytes needed to cover all versions (8-bit chunks)*/
    int set_size; 
    // these are set by data_size method
    int headerSize;
    int groupTableSize;
    int versionTableSize;
    int pairsTableSize;
    int dataTableSize;
};
/**
 * Create an empty MVD
 * @return the MVD or NULL
 */
MVD *mvd_create()
{
    MVD *mvd = calloc( 1, sizeof(MVD) );
    if ( mvd != NULL )
    {
        mvd->versions = dyn_array_create( DEFAULT_VERSION_SIZE );
        mvd->pairs = dyn_array_create( DEFAULT_PAIRS_SIZE );
        mvd->description = "";
        mvd->encoding = DEFAULT_ENCODING;
        mvd->set_size = DEFAULT_SET_SIZE;
        if ( mvd->versions==NULL||mvd->pairs==NULL )
        {
            mvd_dispose( mvd );
            mvd = NULL;
        }
    }
    else
    {
        fprintf(stderr,"MVD: failed to allocate object\n");
    }
    return mvd;
}
/**
 * Dispose of a perhaps partly allocate mvd
 * @param mvd the mvd in question
 * @return NULL
 */
void *mvd_dispose( MVD *mvd )
{
    if ( mvd->versions == NULL )
    {
        int i;
        for ( i=0;i<dyn_array_size(mvd->versions);i++ )
        {
            version *v = dyn_array_get(mvd->versions,i);
            version_dispose( v );
        }
        dyn_array_dispose( mvd->versions );
    }
    if ( mvd->pairs != NULL )
    {
        int i;
        for ( i=0;i<dyn_array_size(mvd->pairs);i++ )
        {
            pair *p = dyn_array_get(mvd->pairs,i);
            pair_dispose(p);
        }
        dyn_array_dispose( mvd->pairs );
    }
    if ( mvd->description != NULL && strlen(mvd->description)>0 )
        free( mvd->description );
    if ( strcmp(mvd->encoding,DEFAULT_ENCODING) != 0 )
        free( mvd->encoding );
    return NULL;
}
/**
 * Compute a hashmap of group names to their IDs (needed for old mvd format)
 * @param mvd the mvd in question
 * @return a map of group names to groups
 */
static hashmap *mvd_get_groups( MVD *mvd )
{
    int i;
    hashmap *groups = hashmap_create( 12, 0 );
    if ( groups != NULL )
    {
        int gid = 1;
        for ( i=0;i<dyn_array_size(mvd->versions);i++ )
        {
            char *parent = NULL;
            char *gname = NULL;
            char *tok = NULL;
            version *v = dyn_array_get(mvd->versions,i);
            char *vid = strdup( version_id(v) );
            if ( vid != NULL )
                tok = strtok( vid, "/" );
            while ( tok != NULL )
            {
                if ( gname != NULL && tok != NULL 
                    && !hashmap_contains(groups,gname) )
                {
                    int pid = 0;
                    if ( parent != NULL )
                    {
                        group *p = hashmap_get( groups, parent );
                        pid = group_id(p);
                    }
                    group *g = group_create( gid++, pid, gname );
                    if ( g != NULL )
                        hashmap_put( groups, gname, g );
                }
                parent = gname;
                gname = tok;
                tok = strtok( NULL, "/" );
            }
            if ( vid != NULL )
                free( vid );
        }
    }
    else
        fprintf(stderr,"mvd: failed to create groups table\n");
    return groups;
}
/**
 * Get the size of the data required in bytes to store this MVD:
 * <p>Header:</p>
 * <ul><li>magic string: 4 bytes must be 0x600DCODE</li>
 * <li>version-table offset: 4-byte int offset from start of file</li>
 * <li>pairs-table offset: 4-byte int offset from start of file</li>
 * <li>data-table offset: 4-byte int offset from start of file</li>
 * <li>description: 2-byte int preceded utf-8 string</li></ul>
 * <li>encoding: 2-byte int preceded utf-8 string</li></ul>
 * @param mvd the MVD in question 
 * @param old equals 1 if the old format is being written
 * @return the byte-size of the serialised mvd or 0 on error
 */
int mvd_datasize( MVD *mvd, int old )
{
    mvd->headerSize = mvd->groupTableSize = mvd->versionTableSize = 
        mvd->pairsTableSize = mvd->dataTableSize = 0;
    // header
    mvd->headerSize = MVD_MAGIC_LEN;
    // table offsets etc
    if ( old )
        mvd->headerSize += 5 * 4; 
    else
        mvd->headerSize += 3 * 4;
    mvd->headerSize += strlen( mvd->description ) + 2;
    mvd->headerSize += strlen( mvd->encoding ) + 2;
    if ( old )
    {
        mvd->groupTableSize = 2; // number of groups
        hashmap *groups = mvd_get_groups( mvd );
        if ( groups != NULL )
        {
            int i,gsize = hashmap_size(groups);
            char **array = calloc( gsize, sizeof(char*) );
            if ( array != NULL )
            {
                hashmap_to_array( groups, array );
                for ( i=0;i<gsize;i++ )
                {
                    group *g = hashmap_get( groups, array[i] );
                    mvd->groupTableSize += group_datasize( g );
                }
                free( array );
            }
            else
            {
                fprintf(stderr,"mvdfile: failed to create hashmap array\n");
                return 0;
            }
            hashmap_dispose( groups, group_dispose );
        }
        else
        {
            fprintf(stderr,"mvdfile: failed to create groups table\n");
            return 0;
        }
    }
    mvd->versionTableSize = 2 + 2; // number of versions + setSize
    int i,vsize = dyn_array_size( mvd->versions );
    int psize = dyn_array_size( mvd->pairs );
    for ( i=0;i<vsize;i++ )
    {
        version *v = dyn_array_get( mvd->versions, i );
        mvd->versionTableSize += version_datasize(v,old);
    }
    mvd->pairsTableSize = 4;	// number of pairs
    mvd->set_size = (dyn_array_size(mvd->versions)+8)/8;
    for ( i=0;i<psize;i++ )
    {
        pair *p = dyn_array_get(mvd->pairs, i );
        mvd->pairsTableSize += pair_size(p,mvd->set_size);
        mvd->dataTableSize += pair_datasize(p);
    }
    return mvd->headerSize + mvd->groupTableSize + mvd->versionTableSize 
        + mvd->pairsTableSize + mvd->dataTableSize;
}
/**
 * Serialise the header in the new format
 * @param mvd the mvd in question
 * @param data the byte array to write to
 * @param len its overall length
 * @param old if 1 create an MVD in the old format
 * @return the number of serialised bytes
 */
static int mvd_serialise_header( MVD *mvd, unsigned char *data, int len, 
    int old ) 
{
    int i,nBytes = 0; 
    char *magic = (old)?MVD_MAGIC_OLD_STR:MVD_MAGIC_NEW_STR;
    for ( i=0;i<MVD_MAGIC_LEN;i++ )
        data[i] = magic[i];
    nBytes = MVD_MAGIC_LEN;
    if ( old )
    {
        // mask type - redundant
        write_int( data, len, nBytes, 0 );
        nBytes += 4;
        // groupTableOffset
        write_int( data, len, nBytes, mvd->headerSize );
        nBytes += 4;
    }
    // versionTableOffset
    write_int( data, len, nBytes, mvd->headerSize+mvd->groupTableSize );
    nBytes += 4;
    // pairsTableOffset
    write_int( data, len, nBytes, mvd->headerSize
        +mvd->groupTableSize
        +mvd->versionTableSize );
    nBytes += 4;
    // dataTableOffset
    write_int( data, len, nBytes, mvd->headerSize
       +mvd->groupTableSize+mvd->versionTableSize
       +mvd->pairsTableSize );
    nBytes += 4;
    nBytes += write_string( data, len, nBytes, mvd->description );
    nBytes += write_string( data, len, nBytes, mvd->encoding );
    return nBytes;
}
/**
 * Serialise the versions table in the old format
 * @param mvd the mvd object
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within data to start writing
 * @return the number of serialised bytes or 0 on failure
 * <li>version-table (new): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: versionID: 2-byte int preceded utf-8 string;
 * version-description: 2-byte preceded utf8 string;  
 * version numbers implied by position in table starting at 1</li>
 */
int mvd_serialise_versions_new( MVD *mvd, char *data, int len, int p )
{
    int i,oldP = p;
    int vsize = dyn_array_size(mvd->versions);
    if ( dyn_array_size(mvd->versions) > 0 )
    {
        write_short( data, len, p, (short)vsize );
        p += 2;
        write_short( data, len, p, (short)mvd->set_size );
        p += 2;
        for ( i=0;i<vsize;i++ )
        {
            version *v = dyn_array_get( mvd->versions, i );
            char *versionID = version_id( v );
            char *description = version_description( v );
            p += write_string( data, len, p, versionID );
            p += write_string( data, len, p, description );
        }
    }
    else
        fprintf(stderr,"mvdfile: no versions defined\n");
    return p - oldP;
}
/**
 * Serialise the versions table in the old format:
 * <li>version-table (old): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: group: 2 byte int; shortName: 2-byte int 
 * preceded utf-8 string; longName: 2-byte int preceded utf-8 string; 
 * version numbers implied by position in table starting at 1</li>
 * @param mvd the mvd object
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within data to start writing
 * @param groups a map of group names to group ids
 * @return the number of serialised bytes or 0 on failure
 */
int mvd_serialise_versions_old( MVD *mvd, char *data, int len, int p, 
    hashmap *groups )
{
    int i,oldP = p;
    int vsize = dyn_array_size(mvd->versions);
    if ( vsize > 0 )
    {
        write_short( data, len, p, (short)vsize );
        p += 2;
        write_short( data, len, p, (short)mvd->set_size );
        p += 2;
        for ( i=0;i<vsize;i++ )
        {
            version *v = dyn_array_get( mvd->versions, i );
            char *gname = NULL;
            char *version = NULL;
            char *tok = NULL;
            char *vid = strdup(version_id(v));
            if ( vid != NULL )
                tok = strtok( vid, "/" );
            while ( tok != NULL )
            {
                gname = version;
                version = tok;
                tok = strtok( NULL, "/" );
            }
            if ( version != NULL )
            {
                int gid = 0;
                if ( gname != NULL )
                {
                    group *g = hashmap_get( groups, gname );
                    gid = group_id( g );
                }
                write_short( data, len, p, (short)gid );
                p += 2;
                write_string( data, len, p, version );
                p += 2 + strlen( version );
                char *longName = version_description(v);
                write_string( data, len, p, longName );
                p += 2 + strlen( longName );
            }
        }
    }
    else
        fprintf(stderr,"mvdfile: no versions defined\n");
    if ( p-oldP != mvd->versionTableSize )
        fprintf(stderr,"mvd: expected versionTableSize %d actual %d\n",
            mvd->versionTableSize,p-oldP);
    return p - oldP;
}
/**
 * Write out the groups in the old format
 * @param data the data to write to
 * @param len the overall length of data
 * @param p the offset to start from
 * @param groups a map of groups to IDs
 * @return the number of bytes written or 0 on error
 * <li>group-table (omitted in new): number of group-definitions: 
 * 2 byte int;<br> for each group: parent: 2-byte int (0 if a top level group); 
 * name: 2-byte int preceded utf-8 string; id implied by position in the table 
 * starting at 1</li>
 */
static int serialise_groups( MVD *mvd, unsigned char *data, int len, int p, 
    hashmap *groups )
{
    int i;
    int oldP = p;
    int gsize = hashmap_size( groups );
    char **keys = calloc(gsize,sizeof(char*));
    char **sorted = calloc(gsize,sizeof(char*));
    if ( sorted != NULL && keys != NULL )
    {
        hashmap_to_array( groups, keys );
        for ( i=0;i<gsize;i++ )
        {
            group *g = hashmap_get( groups, keys[i] );
            int gid = group_id( g );
            if ( gid <= gsize && gid > 0 )
                sorted[gid-1] = keys[i];
            else
            {
                fprintf(stderr,"mvd: invalid group id %d\n",gid);
                goto bail;
            }
        }
        write_short( data, len, p, gsize );
        p += 2;
        // now write them out in order
        for ( i=0;i<gsize;i++ )
        {
            group *g = hashmap_get( groups, sorted[i] );
            int pid = group_parent( g );
            write_short( data, len, p, pid );
            p += 2;
            p += write_string( data, len, p, sorted[i] );
        }
    }
    bail:
    if ( keys != NULL )
        free( keys );
    if ( sorted != NULL )
        free( sorted );
    return p - oldP;
}
/**
 * Fix a data offset in a pair already serialised
 * @param data the data to fix
 * #param len the overall length of data
 * @param p the offset into data where the serialised pair starts
 * @param dataOffset the new value of dataoffset
 * @param vSetSize the number of bytes in the version set
 * @param parentId write this out as the third integer
 */
static void fixDataOffset( unsigned char *data, int len, int p, 
    int dataOffset, int vSetSize, int parentId )
{
    // read versions
    p += vSetSize;
    write_int( data, len, p, dataOffset );
    p += 8;
    write_int( data, len, p, parentId );
}
/**
 * Serialise the pairs table starting at offset p in the data byte 
 * array. Don't serialise the data they refer to yet. Since parents 
 * and children may come in any order we have to keep track of orphaned 
 * children or parents without children, and then join them up when we 
 * can.
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within data to start writing
 * @return the number of serialised bytes, 0 on error
 */
static int mvd_serialise_pairs( MVD *mvd, char *data, int len, int p ) 
{
    int i,nBytes = 0;
    int psize = dyn_array_size(mvd->pairs);
    hashmap *ancestors = hashmap_create( 12, 0 );
    hashmap *orphans = hashmap_create( 12, 0 );
    write_int( data, len, p, psize );
    p += 4;
    nBytes += 4;
    // where we're writing the actual data
    int dataTableOffset = mvd->headerSize+mvd->groupTableSize
        +mvd->versionTableSize+mvd->pairsTableSize;
    int dataOffset = 0;
    int parentDataOffset = 0;
    int parentId = 1;
    for ( i=0;i<psize;i++ )
    {
        // this is set if known
        int tempPId = NULL_PID;
        pair *t = dyn_array_get( mvd->pairs, i );
        if ( pair_is_child(t) )
        {
            // Do we have a registered parent?
            char str[32];
            snprintf( str, 32, "%lx", (long)pair_parent(t) );
            char *value = hashmap_get( ancestors, str );
            // value is the parent's data offset
            if ( value != NULL )
            {
                parentDataOffset = atoi( value );
                pair *parent = pair_parent( t );
                if ( parent != NULL )
                    tempPId = pair_id( parent );
                else
                {
                    fprintf(stderr,"mvd: pair-parent unexpectedly NULL\n");
                    nBytes = 0;
                    break;
                }
            }
            else
            {
                // the value in orphans is the offset 
                // pointing to the orphan pair entry
                char *loc = malloc( 32 );
                if ( loc != NULL )
                {
                    char key[32];// will be duplicated in map
                    snprintf( key, 32, "%lx", (long)t );
                    snprintf( loc, 32, "%d", p );
                    hashmap_put( orphans, key, loc );
                }
                else
                {
                    fprintf(stderr,"mvd: failed to allocate loc string\n");
                    nBytes = 0;
                    break;
                }
                // clearly duff value: fill this in later
                tempPId = DUFF_PID;
            }
        }
        else if ( pair_is_parent(t) )
        {
            char str[32];
            snprintf( str, 32, "%lx", (long)t );
            // first assign this parent an id
            tempPId = pair_set_id( t, parentId++ );
            // then put ourselves in the ancestors list
            char *offset = malloc( 32 );
            if ( offset != NULL )
            {
                snprintf( offset, 32, "%d", dataOffset );
                hashmap_put( ancestors, str, offset );
            }
            else
            {
                fprintf(stderr,"mvd: failed to allocate offset string\n");
                nBytes = 0;
                break;
            }
            // now check if we have any registered orphans
            link_node *node = pair_first_child(t);
            while ( node != NULL )
            {
                pair *child = link_node_obj( node );
                char child_str[32];
                snprintf( child_str, 32, "%lx", (long)child );
                char *value = hashmap_get( orphans, child_str );
                if ( value != NULL )
                {
                    // copy the parent's data offset 
                    // into that of the child
                    int int_val = atoi(value);
                    fixDataOffset( data, len, int_val, dataOffset, 
                        mvd->set_size, pair_id(t) );
                    // remove the child from the orphan list
                    hashmap_remove( orphans, child_str, free );
                }
                node = link_node_next( node );
            }
        }
        // if we set the parent data offset use that
        // otherwise use the current pair's data offset
        nBytes += pair_serialise(t, data, len, p, mvd->set_size, 
            (parentDataOffset!=0)?parentDataOffset:dataOffset, 
            dataTableOffset, tempPId );
        p += pair_size( t, mvd->set_size );
        dataOffset += pair_datasize(t);
        parentDataOffset = 0;
    }
    // ancestors though will be full
    if ( !hashmap_is_empty(orphans) )
    {
        fprintf(stderr,"mvd: unmatched orphans after serialisation\n");
    }
    // can't get rid of parents any other way
    hashmap_dispose( ancestors, free );
    hashmap_dispose( orphans, free );
    return nBytes;
}
/**
 * Serialise the entire mvd into the given byte array. Must 
 * be preceded by a call to dataSize (otherwise no way to 
 * calculate size of data).
 * @param mvd the mvd being serialised
 * @param data a byte array of exactly the right size
 * @param len the overall length of data
 * @param old if 1 write the old MVD format
 * @return the number of serialised bytes
 */
int mvd_serialise( MVD *mvd, unsigned char *data, int len, int old )
{
    int p = mvd_serialise_header( mvd, data, len, old );
    if ( old )
    {
        hashmap *groups = mvd_get_groups( mvd );
        if ( groups != NULL )
        {
            int oldP = p;
            p += serialise_groups( mvd, data, len, p, groups );
            if ( p-oldP != mvd->groupTableSize )
                fprintf(stderr,"mvd: expected groupTableSize %d actual %d\n",
                mvd->groupTableSize,p-oldP);
            p += mvd_serialise_versions_old( mvd, data, len, p, groups );
            // special dispose routine
            hashmap_dispose( groups, group_dispose );
        }
        else
            return 0;
    }
    else
        p += mvd_serialise_versions_new( mvd, data, len, p );
    p += mvd_serialise_pairs( mvd, data, len, p );
    return p;
}
/**
 * Set the size of the bitset required to represent all versions
 * @param mvd the mvd in question
 * @param setSize the size of the set in bytes
 */
void mvd_set_bitset_size( MVD *mvd, int setSize )
{
    mvd->set_size = setSize;
}
/**
 * Get the mvd's set size
 * @param mvd the mvd in question
 * @return the size of the version sets in bits
 */
int mvd_get_set_size( MVD *mvd )
{
    return mvd->set_size;
}
/**
 * Add a new version path to the dynamic list
 * @param mvd the mvd object in question
 * @param v the version (versionID+description)
 */
int mvd_add_version( MVD *mvd, version *v )
{
    return dyn_array_add( mvd->versions, v );
}
/**
 * Get the number of defined versions
 * @param mvd the mvd in question
 * @return the number of versions
 */
int mvd_count_versions( MVD *mvd )
{
    return dyn_array_size(mvd->versions);
}
/**
 * Add a pair to the MVD
 * @param mvd the mvd in question
 * @param tpl2 the pair to add
 */
int mvd_add_pair( MVD *mvd, pair *tpl2 )
{
    return dyn_array_add( mvd->pairs, tpl2 );
}
/** 
 * Set the description string for this MVD
 * @param mvd the mvd in question
 * @param description the desired description string
 * @return 1 if it worked
 */
int mvd_set_description( MVD *mvd, char *description )
{
    if ( mvd->description != NULL && strlen(mvd->description)>0 )
        free( mvd->description );
    mvd->description = strdup( description );
    return mvd->description!= NULL;
}
/** 
 * Set the encoding for this MVD
 * @param mvd the mvd in question
 * @param encoding the desired encoding 
 * @return 1 if it worked
 */
int mvd_set_encoding( MVD *mvd, char *encoding )
{
    if ( mvd->encoding != NULL 
        && strcmp(mvd->encoding,DEFAULT_ENCODING) 
        && strcmp(mvd->encoding,encoding)!=0 )
    {
        free( mvd->encoding );
        mvd->encoding = NULL;
    }
    if ( mvd->encoding == NULL )
        mvd->encoding = strdup( encoding );
    return mvd->encoding!= NULL;
}
/**
 * Debug: check that all pairs have sensible versions
 * @param mvd the mvd to check
 * @return 1 if all was OK
 */
int mvd_check_pairs( MVD *mvd )
{
    int psize = dyn_array_size( mvd->pairs);
    int i;
    for ( i=0;i<psize;i++ )
    {
        pair *p = dyn_array_get( mvd->pairs, i );
        bitset *bs = pair_versions( p );
        if ( bitset_allocated(bs) > 4 )
            printf("stop\n");
    }
}
#ifdef MVD_TEST
/**
 * Test that the versions are all unique
 * @param mvd the mvd to test
 */
int mvd_test_versions( MVD *mvd )
{
    int res = 1;
    int i,vsize = dyn_array_size( mvd->versions );
    hashmap *hm = hashmap_create( vsize, 0 );
    for ( i=0;i<vsize;i++ )
    {
        version *v = dyn_array_get( mvd->versions, i );
        char *id = version_id(v);
        if ( hashmap_contains(hm,id) )
        {
            res = 0;
            fprintf(stderr,"mvd: version %s is non-unique\n",id);
            break;
        }
        else
            hashmap_put( hm, id, "banana" );
    }
    hashmap_dispose( hm, NULL );
    return res;
}
#endif