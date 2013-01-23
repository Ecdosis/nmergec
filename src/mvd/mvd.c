#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#include "mvd/pair.h"
#include "mvd/mvd.h"
#include "dyn_array.h"
#include "mvd/mvdfile.h"
#include "mvd/serialiser.h"
#include "mvd/group.h"
#include "utils.h"
#include "hashmap.h"

#define DEFAULT_VERSION_SIZE 32
#define DEFAULT_PAIRS_SIZE 4096
#define DEFAULT_ENCODING "UTF-8"
#define DEFAULT_SET_SIZE 32
#define DUFF_PID -1
#define NULL_PID 0
	
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
            char *v = dyn_array_get(mvd->versions,i);
            free(v);
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
int mvd_datasize( MVD *mvd )
{
    return 1;
}
/**
 * Serialise the header in the new format
 * @param mvd the mvd in question
 * @param data the byte array to write to
 * @param len its overall length
 * @param old if 1 create an MVD in the old format
 * @return the number of serialised bytes
 */
static int mvd_serialise_header( MVD *mvd, unsigned char *data, int old ) 
{
    int i,nBytes = 0; 
    char *magic = (old)?MVD_MAGIC_OLD_STR:MVD_MAGIC_NEW_STR;
    for ( i=0;i<MVD_MAGIC_LEN;i++ )
        data[i] = magic[i];
    nBytes = MVD_MAGIC_LEN;
    if ( old )
    {
        // mask type - redundant
        write_int( data, nBytes, 0 );
        nBytes += 4;
        // groupTableOffset
        write_int( data, nBytes, mvd->headerSize );
        nBytes += 4;
    }
    // versionTableOffset
    write_int( data, nBytes, mvd->headerSize+mvd->groupTableSize );
    nBytes += 4;
    // pairsTableOffset
    write_int( data, nBytes, mvd->headerSize
        +mvd->groupTableSize
        +mvd->versionTableSize );
    nBytes += 4;
    // dataTableOffset
    write_int( data, nBytes, mvd->headerSize
       +mvd->groupTableSize+mvd->versionTableSize
       +mvd->pairsTableSize );
    nBytes += 4;
    nBytes += write_string( data, nBytes, mvd->description );
    nBytes += write_string( data, nBytes, mvd->encoding );
   return nBytes;
}
/**
 * Serialise the versions table in the old format
 * @param mvd the mvd object
 * @param data the byte array to write to
 * @param p the offset within data to start writing
 * @return the number of serialised bytes or 0 on failure
 * <li>version-table (new): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: versionID: 2-byte int preceded utf-8 string;  
 * version numbers implied by position in table starting at 1</li>
 */
int mvd_serialise_versions_new( MVD *mvd, char *data, int p )
{
    int i,oldP = p;
    int vsize = dyn_array_size(mvd->versions);
    if ( dyn_array_size(mvd->versions) > 0 )
    {
        write_short( data, p, (short)vsize );
        p += 2;
        write_short( data, p, (short)mvd->set_size );
        p += 2;
        for ( i=0;i<vsize;i++ )
        {
            char *vID = dyn_array_get( mvd->versions, i );
            write_string( data, p, vID );
            p += 2 + strlen( vID );
        }
    }
    else
        fprintf(stderr,"mvdfile: no versions defined\n");
    return p - oldP;
}
/**
 * Serialise the versions table in the old format
 * @param mvd the mvd object
 * @param data the byte array to write to
 * @param p the offset within data to start writing
 * @param groups a map of group names to group ids
 * @return the number of serialised bytes or 0 on failure
 * <li>version-table (old): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: group: 2 byte int; shortName: 2-byte int 
 * preceded utf-8 string; longName: 2-byte int preceded utf-8 string; 
 * version numbers implied by position in table starting at 1</li>
 */
int mvd_serialise_versions_old( MVD *mvd, char *data, int p, hashmap *groups )
{
    int i,oldP = p;
    int vsize = dyn_array_size(mvd->versions);
    if ( dyn_array_size(mvd->versions) > 0 )
    {
        write_short( data, p, (short)vsize );
        p += 2;
        write_short( data, p, (short)mvd->set_size );
        p += 2;
        for ( i=0;i<vsize;i++ )
        {
            char *v = dyn_array_get( mvd->versions, i );
            char *group = NULL;
            char *version = NULL;
            char *tok = strtok( v, "/" );
            while ( tok != NULL )
            {
                group = version;
                version = tok;
                tok = strtok( NULL, "/" );
            }
            if ( version != NULL )
            {
                int gid = (group==NULL)?0:atoi(hashmap_get(groups,group));
                write_short( data, p, (short)gid );
                p += 2;
                write_string( data, p, version );
                p += 2 + strlen( version );
                int l_len = strlen(version)+strlen("Version ")+1;
                char *longName = malloc(l_len);
                if ( longName != NULL )
                {
                    snprintf(longName,l_len,"Version %s",version);
                    write_string( data, p, longName );
                    p += 2 + strlen( longName );
                    free( longName );
                }
                else
                {
                    fprintf(stderr,"mvd: missing long name for version %s\n",
                        version);
                    write_string(data, p, "");
                }
            }
        }
    }
    else
        fprintf(stderr,"mvdfile: no versions defined\n");
    return p - oldP;
}
/**
 * Free the values of a hashmap of groups. The values need deallocating.
 * @param groups the hashmap in question
 */
static void free_groups( hashmap *groups )
{
    char **keys = calloc( hashmap_size(groups),sizeof(char*));
    if ( keys != NULL )
    {
        int i;
        hashmap_to_array( groups, keys );
        for ( i=0;i<hashmap_size(groups);i++ )
        {
            group *g = hashmap_get( groups,keys[i] );
            if ( g != NULL )
                group_dispose( g );
        }
        hashmap_dispose( groups );
        free( keys );
    }
    else
        fprintf(stderr,"mvd: failed to deallocate groups table\n");
}
/**
 * Compute a hashmap of group names to their IDs (needed for old mvd format)
 * @param mvd the mvd in question
 * @return a map of group names and IDs. IDs must be deallocated
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
            char *versionID = dyn_array_get(mvd->versions,i);
            char *tok = strtok( versionID, "/" );
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
                    group *g = group_create( gid++, pid );
                    if ( g != NULL )
                        hashmap_put( groups, gname, g );
                }
                parent = gname;
                gname = tok;
                tok = strtok( NULL, "/" );
            }
        }
    }
    else
        fprintf(stderr,"mvd: failed to create groups table\n");
    return groups;
}
/**
 * Write out the groups in the old format
 * @param data the data to write to
 * @param p the offset to start from
 * @param groups a map of groups to IDs
 * @return the number of bytes written or 0 on error
 * <li>group-table (omitted in new): number of group-definitions: 
 * 2 byte int;<br> for each group: parent: 2-byte int (0 if a top level group); 
 * name: 2-byte int preceded utf-8 string; id implied by position in the table 
 * starting at 1</li>
 */
static int serialise_groups( unsigned char *data, int p, hashmap *groups )
{
    int i;
    int gsize = hashmap_size( groups );
    char **keys = calloc(gsize,sizeof(char*));
    char **sorted = calloc(gsize,sizeof(char*));
    if ( sorted != NULL && keys != NULL )
    {
        hashmap_to_array( groups, keys );
        for ( i=0;i<hashmap_size(groups);i++ )
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
        write_short( data, p, gsize );
        p += 2;
        // now write them out in order
        for ( i=0;i<gsize;i++ )
        {
            group *g = hashmap_get( groups, sorted[i] );
            int pid = group_id( g );
            write_short( data, p, pid );
            p += 2;
            write_string( data, p, sorted[i] );
            p += 2 + strlen(sorted[i]);
        }
    }
    bail:
    if ( keys != NULL )
        free( keys );
    if ( sorted != NULL )
        free( sorted );
}
/**
 * Serialise the pairs table starting at offset p in the data byte 
 * array. Don't serialise the data they refer to yet. Since parents 
 * and children may come in any order we have to keep track of orphaned 
 * children or parents without children, and then join them up when we 
 * can.
 * @param data the byte array to write to
 * @param p the offset within data to start writing
 * @return the number of serialised bytes
 */
static int mvd_serialise_pairs( MVD *mvd, char *data, int p ) 
{
    int nBytes = 0;
    int psize = dyn_array_size(mvd->pairs);
    hashmap *ancestors = hashmap_create( 12, 0 );
    hashmap *orphans = hashmap_create( 12, 0 );
    write_int( data, p, psize );
    p += 4;
    nBytes += 4;
    // where we're writing the actual data
    int dataTableOffset = mvd->headerSize+mvd->groupTableSize
        +mvd->versionTableSize+mvd->pairsTableSize;
    int dataOffset = 0;
    int parentDataOffset = 0;
    int parentId = 1;
    /*for ( int i=0;i<psize;i++ )
    {
        // this is set if known
        int tempPId = NULL_PID;
        pair *t = dyn_array_get( mvd->pairs, i );
        if ( pair_is_child(t) )
        {
            // Do we have a registered parent?
            Integer value = ancestors.get( t.parent );
            // value is the parent's data offset
            if ( value != null )
            {
                parentDataOffset = value.intValue();
                tempPId = t.parent.id;
            }
            else
            {
                // the value in orphans is the offset 
                // pointing to the orphan pair entry
                orphans.put( t, new Integer(p) );
                // clearly duff value: fill this in later
                tempPId = DUFF_PID;
            }
        }
        else if ( t.isParent() )
        {
            // first assign this parent an id
            tempPId = t.id = parentId++;
            // then put ourselves in the ancestors list
            ancestors.put( t, dataOffset );
            // now check if we have any registered orphans
            ListIterator<Pair> iter = t.getChildIterator();
            while ( iter.hasNext() )
            {
                Pair child = iter.next();
                Integer value = orphans.get( child );
                if ( value != null )
                {
                    // copy the parent's data offset 
                    // into that of the child
                    Pair.fixDataOffset( data, value.intValue(), 
                        dataOffset, versionSetSize, t.id );
                    // remove the child from the orphan list
                    orphans.remove( child );
                }
            }
        }
        // if we set the parent data offset use that
        // otherwise use the current pair's data offset
        nBytes += t.serialisePair( data, p, versionSetSize, 
            (parentDataOffset!=0)?parentDataOffset:dataOffset, 
            dataTableOffset, tempPId );
        p += t.pairSize( versionSetSize );
        dataOffset += t.dataSize();
        parentDataOffset = 0;
    }
    if ( !orphans.isEmpty() )
    {
        Set<Pair> keys = orphans.keySet();
        Iterator<Pair> iter = keys.iterator();
        while ( iter.hasNext() )
        {
            Pair q = iter.next();
            if ( !ancestors.containsKey(q) )
                System.out.println("No matching key for pair");
        }
        throw new MVDException("Unmatched orphans after serialisation");
    }*/
   return nBytes;
}
/**
 * Serialise the entire mvd into the given byte array. Must 
 * be preceded by a call to dataSize (otherwise no way to 
 * calculate size of data).
 * @param data a byte array of exactly the right size
 * @param old if 1 write the old MVD format
 * @return the number of serialised bytes
 */
int mvd_serialise( MVD *mvd, unsigned char *data, int old )
{
    int p = mvd_serialise_header( mvd, data, old );
    if ( old )
    {
        hashmap *groups = mvd_get_groups( mvd );
        if ( groups != NULL )
        {
            p += serialise_groups( data, p, groups );
            p += mvd_serialise_versions_old( mvd, data, p, groups );
            // special dispose routine
            free_groups( groups );
        }
        else
            return 0;
    }
    else
        p += mvd_serialise_versions_new( mvd, data, p );
    p += mvd_serialise_pairs( mvd, data, p );
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
 * @param versionID the version path uniquely identifying the version
 */
int mvd_add_version_path( MVD *mvd, char *versionID )
{
    int res = 0;
    char *copy = strdup(versionID);
    if ( copy != NULL )
        res = dyn_array_add( mvd->versions, copy );
    return res;
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