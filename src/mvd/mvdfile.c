#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "unicode/uchar.h"
#include "benchmark.h"
#include "char_buf.h"
#include "zip/zip.h"
#include "bitset.h"
#include "link_node.h"
#include "mvd/pair.h"
#include "mvd/version.h"
#include "mvd/mvd.h"
#include "dyn_array.h"
#include "dyn_string.h"
#include "b64.h"
#include "hashmap.h"
#include "utils.h"
#include "mvd/mvdfile.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define KEYLEN 16

/**
 * <h4>MVD file format</h4>
 * <ul><li>outer wrapper: base64 encoding</li>
 * <li>inner wrapper: zip compression</li></ul>
 * <p>Header:</p>
 * <ul><li>magic string: 4 bytes must be 0xDEADC0DE (old) or 0x600DCODE (new)</li>
 * <li>data mask: 4 bytes 0=NONE (omitted in new)</li>
 * <li>group-table offset: 4-byte int offset from start of file(omitted in new)</li>
 * <li>version-table offset: 4-byte int offset from start of file</li>
 * <li>pairs-table offset: 4-byte int offset from start of file</li>
 * <li>data-table offset: 4-byte int offset from start of file</li>
 * <li>description: 2-byte int preceded string</li></ul>
 * <li>encoding: 2-byte int preceded string. All strings in the MVD follow 
 * THIS encoding</li></ul>
 * <p>Tables:</p>
 * <ul><li>group-table (omitted in new): number of group-definitions: 
 * 2 byte int;<br> for each group: parent: 2-byte int (0 if a top level group); 
 * name: 2-byte int preceded string; id implied by position in the table 
 * starting at 1</li>
 * <li>version-table: number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: group: 2 byte int; shortName(old)/versionID(new): 
 * 2-byte int preceded string; longName: 2-byte int preceded string; 
 * version numbers implied by position in table starting at 1</li>
 * <li>pairs table: number of pairs: 4-byte int;<br/>
 * for each pair: version-set size bytes, LSB first. first bit of 
 * first byte is hint bit (all other bits refer to versions 1 up to number  
 * of versions); data offset: 4-byte unsigned indexing into data-table; 
 * data len: 4-byte unsigned, first 2 bits forming the transpose flag 
 * 0=DATA,1=CHILD,2=PARENT. If PARENT or CHILD an extra integer containing 
 * the ID of the parent or that of the child's parent.</li>
 * <li>data-table: format: raw bytes</li></ul>
 * <p>all ints are signed big-endian as per Java VM</p>
 */
/**
 * Extract the MVD magic code at the start of every MVD file
 * @param mvd_data the uncompressed mvd data structure
 * @return the mvd_magic value
 */
static unsigned mvd_magic( unsigned char *mvd_data )
{
    unsigned value=0;
    int i;
    // watch out for those endianness bugs....
    for ( i=0;i<4;i++ )
    {
        value <<= 8;
        value |= mvd_data[i];
    }
    return value;
}
/**
 * Read a 4-byte integer from an array of bytes in big-endian order
 * @param data an array of bytes
 * @param len the length of data
 * @param p offset into data to begin
 * @return the int read from data
 */
static int readInt( unsigned char *data, int len, int p ) 
{
   int i, x = 0;
   if ( p+3 < len )
   {
       for ( i=p;i<p+4;i++ )
       {
           x <<= 8;
           x |= 0xFF & data[i];
       }
   }
   return x;
}
/**
 * Read a 2-byte integer from an array of bytes in big-endian order
 * @param data an array of bytes
 * @param len the length of data
 * @param p offset into data to begin
 * @return the short read from data
 */
static short readShort( unsigned char *data, int len, int p ) 
{
   short i,x = 0;
   if ( p+1 < len )
   {
       for ( i=p;i<p+2;i++ )
       {
           x <<= 8;
           x = data[i];
       }
   }
   return x;
}
/**
 * Read a plain vanilla ASCII string
 * @param data the data to read from
 * @param len the maximum length of that data
 * @param slen VAR param set to length of ascii string
 * @param p the offset into data
 * @return the allocated string or NULL on failure
 */
static char *readAsciiString( unsigned char *data, int len, short *slen, int p )
{
    short nchars = readShort( data, len, p );
    char *str = NULL;
    p += 2;
    if ( nchars+p<=len )
    {
        str = calloc( nchars+1, 1 );
        if ( str != NULL )
            memcpy( str, &data[p], nchars );
        else
            fprintf(stderr,"mvdfile: failed to copy ascii string\n");
    }
    else
        fprintf(stderr,"mvdfile: data overflow\n");
    *slen = (int) nchars;
    return str;
}     
/**
 * Read a 2-byte preceded string from an array of data bytes
 * @param data the data to read from
 * @param len the overall length of the mvd_data
 * @param dlen VAR param update length of data read or 0 on failure
 * @param p the offset of the string start
 * @param encoding the encoding of the data
 * @return an allocated UChar string or NULL
 */
static UChar *readEncodedString( unsigned char *data, int len, 
    short *dlen, int p, char *encoding ) 
{
    UChar *str = NULL;
    short slen = readShort( data, len, p );
    p += 2;
    if ( slen+p <= len )
    {
        int nchars = measure_from_encoding(&data[p],slen,encoding)/2;
        str = calloc( nchars+1,sizeof(UChar) );
        if ( str != NULL )
        {
            nchars = convert_from_encoding(&data[p], slen, str, nchars+1, 
                encoding)/sizeof(UChar);
            if ( str != NULL )
            {
                *dlen = slen;
            }
            else
            {
                fprintf(stderr,"mvdfile: failed to allocate wide string\n");
                *dlen = 0;
            }
        }
        else
            fprintf(stderr,"encoding: failed to allocate string\n");
    }
    return str;
}
/**
 * Read the group table for an MVD from a byte array
 * @param mvd the mvd object
 * @param data the byte array containing the group definitions
 * @param len the length of the mvd_data
 * @param p the start offset of the groups within data
 * @param return an allocated array of group paths or NULL
 */
static UChar **readGroupTable( MVD *mvd, unsigned char *mvd_data, 
    int len, int p )
{
    short nGroups = readShort( mvd_data, len, p );
    p += 2;
    if ( nGroups < 0 )
    {
        fprintf(stderr,"mvdfile: invalid number (%d) of groups: ",nGroups );
        return NULL;
    }
    else
    {
        UChar **actual_paths = NULL;
        //printf("nGroups=%d\n",nGroups);
        dyn_string **paths = calloc(nGroups,sizeof(dyn_string*));
        if ( paths != NULL )
        {
            int i;
            int res = 1;
            for ( i=0;i<nGroups;i++ )
            {
                char buf[64];
                short dlen,parent = readShort( mvd_data, len, p );
                //printf("parent=%d\n",parent);
                assert( parent <= i && parent < nGroups );
                p += 2;
                UChar *name = readEncodedString( mvd_data, len, &dlen, 
                    p, mvd_get_encoding(mvd) );
                //printf( "name=%s\n",u_print(name,buf,64) );
                p += 2 + dlen;
                if ( name != NULL )
                {
                    paths[i] = dyn_string_create();
                    if (paths[i] == NULL )
                        res = 0;
                    else if ( parent == 0 )
                        res = dyn_string_concat( paths[i], name );
                    else
                    {
                        dyn_string *pt = paths[parent];
                        int res = dyn_string_concat(paths[i],
                            dyn_string_data(pt));
                        if ( res )
                            res = dyn_string_concat(paths[i],SLASH);
                        if ( res )
                            res = dyn_string_concat(paths[i],name);
                    }
                    free( name );
                }
                if ( !res )
                    break;
            }
            if ( res )
            {
                actual_paths = calloc( nGroups+1,sizeof(UChar*) );
                // copy dynamic to actual strings - still must be freed
                for ( i=0;i<nGroups;i++ )
                {
                    actual_paths[i] = u_strdup(dyn_string_data(paths[i]));
                    if ( actual_paths[i] == NULL )
                    {
                        // clean up...
                        int j;
                        for ( j=0;j<i;j++ )
                            free(actual_paths[j]);
                        free( actual_paths );
                        actual_paths = NULL;
                        break;
                    }
                }
            }
            for ( i=0;i<nGroups;i++ )
                if ( paths[i] != NULL )
                    dyn_string_dispose(paths[i]);
            free( paths );
        }
        return actual_paths;
    }
}
/**
 * Read a new-style version table
 * Format: number of versions: 2-byte int; 
 * version-set size: 2-byte int;
 * for each version: versionID: 2-byte int preceded string;  
 * version numbers implied by position in table starting at 1.
 * @param mvd_data the binary mvd data
 * @param p the offset into mvd_data to start from
 * @param len the overall length of the mvd_data
 * @param mvd the mvd object to store the version table in
 * @return 1 if it worked, else 0
 */
static int readNewVersionTable( unsigned char *mvd_data, int p, int len, 
    MVD *mvd )
{
    int i,res = 1;
    short nVersions = readShort( mvd_data, len, p );
    p += 2;
    if ( nVersions < 0 )
    {
        fprintf(stderr,"mvdfile: invalid number (%d) of versions: ",
            nVersions );
        res = 0;
    }
    else
    {
        char buf[64];
        short setSize = readShort( mvd_data, len, p );
        p += 2;
        mvd_set_bitset_size( mvd, setSize );
        for ( i=0;i<nVersions;i++ )
        {
            // read versionID
            UChar *description,*versionID;
            short dlen;
            version *v = NULL;
            //unsigned char *data, int len, short *dlen, int p, char *encoding
            versionID = readEncodedString( mvd_data, len, &dlen, p, 
                mvd_get_encoding(mvd) );
            //printf("versionID=%s\n",u_print(versionID,buf,64));
            p += 2 + dlen;
            // read description
            description = readEncodedString( mvd_data, len, &dlen, p, 
                mvd_get_encoding(mvd) );
            p += 2 + dlen;
            //printf("description=%s\n",u_print(description,buf,64));
            if ( versionID != NULL && description != NULL )
                v = version_create( versionID, description );
            if ( v != NULL )
                res = mvd_add_version( mvd, v );
            if ( versionID != NULL )
                free( versionID );
            if ( description != NULL )
                free( description );
            if ( v == NULL )
            {
                fprintf(stderr,"mvdfile: bad versionID. len=%d\n",dlen);
                res = 0;
            }
            if ( !res )
                break;
        }
    }
    return res;
}
/**
 * Read a version table in the old format
 * @param mvd_data the raw mvd bytes
 * @param len the overall length of mvd_data
 * @param p the offset into mvd_data to start from
 * @param groups a NULL-terminated array of group paths
 * @param mvd the mvd to store the completed version paths in
 * @return 1 if it worked else 0
 */
static int readOldVersionTable( unsigned char *mvd_data, int len, int p, 
    UChar **groups, MVD *mvd )
{
    int oldP = p;
    int i,res = 1;
    short nVersions = readShort( mvd_data, len, p );
    p += 2;
    if ( nVersions < 0 )
    {
        fprintf(stderr,"mvdfile: invalid number (%d) of versions: ",
            nVersions );
        res = 0;
    }
    else
    {
        short setSize = readShort( mvd_data, len, p );
        p += 2;
        mvd_set_bitset_size( mvd, setSize );
        for ( i=0;i<nVersions;i++ )
        {
            short group = readShort( mvd_data, len, p );
            // default is first group
            if ( group == 0 )
                group = 1;
            p += 2;
            // ignore backup
            short dlen,backup = readShort( mvd_data, len, p );
            p += 2;
            char buf[128];
            UChar *shortName,*longName;
            shortName = readEncodedString( mvd_data, len, &dlen, p, 
                mvd_get_encoding(mvd) );
            //printf("shortName=%s\n",u_print(shortName,buf,128));
            p += 2 + dlen;
            longName = readEncodedString( mvd_data, len, &dlen, p, 
                mvd_get_encoding(mvd) );
            //printf("longName=%s\n",u_print(longName,buf,128));
            p += 2 + dlen;
            if ( shortName != NULL && longName != NULL )
            {
                dyn_string *ds = dyn_string_create_from( groups[group-1] );
                if ( ds != NULL )
                {
                    res = dyn_string_concat( ds, SLASH );
                    if ( res )
                        res = dyn_string_concat( ds, shortName );
                    if ( res )
                    {
                        char buf[64];
                        version *v = version_create( dyn_string_data(ds), 
                            longName );
                        //printf("versionID=%s\n",u_print(dyn_string_data(ds),
                        //    buf,64));
                        if ( v != NULL )
                            res = mvd_add_version( mvd, v );
                        else
                            res = 0;
                    }
                    dyn_string_dispose( ds );
                }
            }
            else
                res = 0;
            if ( shortName != NULL )
                free( shortName );
            if ( longName != NULL )
                free( longName );
        }
    }
    //printf("p-oldP=%d\n",(p-oldP));
    return res;
}
/**
 * Read a version set MSB first and convert it to a bitset
 * @param setSize number of bytes in the bitset
 * @param data the data to read it from
 * @param len the overall length of data
 * @param p offset within data to start
 * @return the finished BitSet or NULL
 */
static bitset *readVersionSet( int setSize, unsigned char *data, 
    int len, int p )
{
    bitset *versions = bitset_create_exact( setSize*8 );
    if ( versions != NULL )
    {
        int j,k;
        p += setSize-1;
        for ( j=0;j<setSize;j++,p-- )
        {
            unsigned char mask = (unsigned char)1;
            for ( k=0;k<8;k++ )
            {
                if ( (mask & data[p]) != 0 )
                    bitset_set(versions,k+(j*8) );
                mask <<= 1;
            }
        }
    }
    return versions;
}
/**
 * Read a pair's data by converting it to utf-16
 * @param data a direct pointer to the data in the encoding
 * @param data_len the data's length in encoded form
 * @param ulen VAR param set to length of decoded data in UChars
 * @param encoding the canonical name of the encoding in lowercase
 * @return a pointer to the allocated data or NULL if empty
 */
static UChar *read_pair_data( unsigned char *data, int data_len, int *ulen, 
    char *encoding )
{
    UChar *dst = NULL;
    *ulen = 0;
    if ( data_len > 0 )
    {
        int dstLen = measure_from_encoding( data, data_len, encoding );
        dst = calloc( dstLen+1, sizeof(UChar) );
        if ( dst != NULL )
            *ulen = convert_from_encoding(data,data_len,dst,dstLen, 
                encoding)/sizeof(UChar);
        else
            fprintf(stderr,"mvdfile: failed to convert pair data\n");
    }
    return dst;
}
/**
 * Read the pairs table for an MVD from a byte array
 * @param data the byte array containing the pairs definitions
 * @param len the overall length of the mvd_data
 * @param p the start offset of the pairs within data
 * @param dataTableOffset offset within data of the pairs data 
 * @param mvd an mvd to add the version definitions to
 * @return 1 if it worked
 */
static int readPairsTable( unsigned char *mvd_data, int len,
    int p, int dataTableOffset, MVD *mvd )
{
    int res = 0;
    int parentDataLen=0;
    int directDataLen=0;
    bitset *versions = NULL;
    // record any pairs declaring themselves as parents
    hashmap *parents = hashmap_create( 128, 1 );
    hashmap *orphans = hashmap_create( 128, 1 );
    if ( parents != NULL && orphans != NULL )
    {
        int i,nPairs = readInt( mvd_data, len, p );
        p += 4;
        if ( nPairs < 0 )
        {
            fprintf(stderr,"mvdfile: invalid number (%d) of pairs: ", nPairs );
            return 0;
        }
        int setSize = mvd_get_set_size(mvd);
        for ( i=0;i<nPairs;i++ )
        {
            UChar u_key[KEYLEN];
            pair *tpl2;
            versions = readVersionSet( setSize, mvd_data, len, p );
            if ( versions == NULL )
                break;
            p += setSize;
            int data_offset = readInt( mvd_data, len, p );
            p += 4;
            int data_len = readInt( mvd_data, len, p );
            int flag = data_len & TRANSPOSE_MASK;
            // clear top two bits
            data_len &= INVERSE_MASK;
            p += 4;
            if ( flag == PARENT_FLAG )
            {
                int offset = dataTableOffset+data_offset;
                // read special parent id field 
                int pId = readInt( mvd_data, len, p );
                parentDataLen += data_len;
                p += 4;
                // transpose parent
                int ulen;
                UChar *pair_data = read_pair_data( &mvd_data[offset], 
                    data_len, &ulen, mvd_get_encoding(mvd) ); 
                tpl2 = pair_create_parent( versions, pair_data, ulen );
                if ( pair_data != NULL )
                    free( pair_data );
                calc_ukey( u_key, pId, KEYLEN );
                // check for orphans of this parent
                link_node *children = hashmap_get( orphans, u_key );
                if ( children != NULL )
                {
                    link_node *temp = children;
                    // match them up with this parent
                    do
                    {
                        pair *child = link_node_obj(temp);
                        tpl2 = pair_add_child( tpl2, child );
                        // tpl2 may have changed...
                        if ( tpl2 == NULL )
                            break;
                        else
                            pair_set_parent( child, tpl2 );
                    } while ( (temp=link_node_next(temp))!= NULL );
                    // now they're not orphans any more. hooray!
                    int removed = hashmap_remove( orphans, u_key, NULL );
                    link_node_dispose( children );
                    assert(removed==1);
                }
                // always do this in case more children turn up
                hashmap_put( parents, (UChar*)u_key, tpl2 );
            }
            else if ( flag == CHILD_FLAG )
            {
                // read special parent id field 
                int pId = readInt( mvd_data, len, p );
                p += 4;
                // transpose child
                calc_ukey( u_key, pId, KEYLEN );
                pair *parent = hashmap_get( parents, u_key );
                if ( parent == NULL )
                {
                    tpl2 = pair_create_child( versions );
                    link_node *ln = link_node_create();
                    if ( ln == NULL || tpl2==NULL )
                        break;
                    link_node_set_obj( ln, tpl2 );
                    link_node *children = hashmap_get( orphans, u_key );   
                    if ( children == NULL )
                    {
                        res = hashmap_put(orphans,u_key,ln);
                        if ( !res )
                            break;
                    }
                    else
                        link_node_append( children, ln );
                }
                else	// parent available
                {
                    tpl2 = pair_create_child( versions );
                    if ( tpl2 == NULL )
                        break;
                    pair_add_child( parent, tpl2 );
                    pair_set_parent( tpl2, parent );
                }
            }
            else // no transposition
            {
                int offset = dataTableOffset+data_offset;
                int ulen;
                directDataLen += data_len;
                UChar *pair_data = read_pair_data( &mvd_data[offset], 
                    data_len, &ulen, mvd_get_encoding(mvd) ); 
                tpl2 = pair_create_basic( versions, pair_data, ulen );
                if ( pair_data != NULL )
                    free( pair_data );
                if ( tpl2 == NULL )
                    break;
            }
            if ( i == 2018 )
                printf("2018!\n");
            if ( !mvd_add_pair(mvd,tpl2) )
                break;
            versions = bitset_dispose( versions );
        }
        res = (i== nPairs);
    }
    printf("direct=%d parent=%d\n",directDataLen,parentDataLen);
    return res;
}
/**
 * Parse an old format MVD
 * @param mvd_data the mvd_data to parse
 * @param len t=its length
 * @param old 1 if this is the old format
 * @return the read MVD
 */
MVD *mvd_parse( unsigned char *mvd_data, int len, int old )
{
    UChar *description;
    char *encoding;
    int res = 0;
    short slen;
    int groupTableOffset,maskType;
    MVD *mvd = NULL;
    // point after magic
    int p = 4;
    // mask type - redundant
    if ( old )
    {
        maskType = readInt( mvd_data, len, p );
        p += 4;
        groupTableOffset = readInt( mvd_data, len, p );
        p += 4;
    }
    int versionTableOffset = readInt( mvd_data, len, p );
    p += 4;
    int pairsTableOffset = readInt( mvd_data, len, p );
    p += 4;
    int dataTableOffset = readInt( mvd_data, len, p );
    p += 4;
    if ( old )
    {
        char buf[64];
        // skip over description for now
        int desc_p = p;
        int desc_len = readShort( mvd_data, len, p );
        p += 2 + desc_len;
        // encoding is needed to read all other strings, the quid pro quo
        encoding = readAsciiString( mvd_data, len, &slen, p );
        lowercase( encoding );
        description = readEncodedString( mvd_data, len, &slen, desc_p, encoding );
        //printf("description=%s\n",u_print(description,buf,64));
    }
    else
    {
        char buf[64];
        encoding = readAsciiString( mvd_data, len, &slen, p );
        p += 2 + slen;
        description = readEncodedString( mvd_data, len, &slen, p, encoding );
        //printf("description=%s\n",u_print(description,buf,64));
    }
    // end of header
    mvd = mvd_create();
    if ( description != NULL )
    {
        res = mvd_set_description( mvd, description );
        free( description );
    }
    if ( res && encoding != NULL )
    {
        res = mvd_set_encoding( mvd, encoding );
        free( encoding );
    }
    if ( res )
    {
        if ( old )
        {
            p = groupTableOffset;
            UChar **groups = readGroupTable( mvd, mvd_data, len, p );
            if ( groups != NULL )
            {
                p = versionTableOffset;
                res = readOldVersionTable( mvd_data, len, p, groups, mvd );
                p = pairsTableOffset;
            }
            else 
                res = 0;
        }
        else
        {
            p = versionTableOffset;
            res = readNewVersionTable( mvd_data, p, len, mvd );
            p = pairsTableOffset;
        }
        if ( res ) 
            res = readPairsTable( mvd_data, len, p, dataTableOffset, mvd );
    }
    if ( !res )
    {
        fprintf(stderr,"mvdfile: failed to read pairs table\n");
        mvd = mvd_dispose( mvd );
    }
    return mvd;
}
/**
 * Read an MVD into memory from its textual representation
 * @param data the MVD in base 64
 * @param len its length
 * @return a loaded MVD or NULL
 */
MVD *mvd_internalise( char *data, int len ) 
{
    int mvd_len;
    //long start,end;
    //long start_mem,end_mem;
    //start = epoch_time();
    //start_mem = get_mem_usage();
    MVD *mvd = NULL;
    size_t zip_len = b64_decode_buflen( (size_t)len );
    if ( zip_len > 0 )
    {
        unsigned char *zip_data = malloc( zip_len );
        if ( zip_data != NULL )
        {
            b64_decode( data, len, zip_data, zip_len ); 
            char_buf *buf = char_buf_create( zip_len*2 );
            if ( buf != NULL )
            {
                int res = zip_inflate( zip_data, zip_len, buf );
                if ( res )
                {
                    unsigned char *mvd_data = char_buf_get( buf, &mvd_len );
                    mvd = mvd_parse( mvd_data, mvd_len, 
                        mvd_magic(mvd_data)==MVD_MAGIC_OLD );
                }
                else
                    fprintf(stderr,"mvdfile: zip inflate failed\n");
                //end_mem = get_mem_usage();
                char_buf_dispose( buf );
            }
            free( zip_data );
        }
        else
            fprintf(stderr,"mvdfile: failed to allocate zip buffer\n");
    }
    //end = epoch_time();
    //printf("time taken to internalise: %ld microsseconds\n",(end-start));
    //printf("memory used in internalise: %ld bytes\n",(end_mem-start_mem));
    return mvd;
}
/**
 * Write an MVD out to a string in base 64
 * @param mvd the mvd to externalise
 * @param len VAR param set to length of returned data
 * @param old write the old format of MVD
 * @return the data as an allocated base64 string, caller to free, or NULL
 */
char *mvdfile_externalise( MVD *mvd, int *len, int old )
{
    char *b64_data = NULL;
    int size = mvd_datasize( mvd, old );
    unsigned char *data = malloc( size );
    //long end_mem,start_mem = get_mem_usage();
    if ( data != NULL )
    {
        int nBytes = mvd_serialise( mvd, data, size, old );
        if ( nBytes < size )
            fprintf(stderr,"mvdfile: MVD shorter than predicted\n");
        else
        {
            char_buf *zip_buf = char_buf_create( nBytes );
            if ( zip_buf != NULL )
            {
                int res = zip_deflate( data, nBytes, zip_buf );
                if ( res )
                {
                    int zip_len;
                    unsigned char *zip_data = char_buf_get( zip_buf, &zip_len );
                    size_t b64_len = b64_encode_buflen( zip_len );
                    b64_data = malloc( b64_len+1 );
                    if ( b64_data != NULL )
                    {
                        b64_encode(zip_data, zip_len, b64_data, b64_len );
                        *len = b64_len;
                        //end_mem = get_mem_usage();
                    }
                }
                char_buf_dispose( zip_buf );
            }
        }
        free( data );
    }
    return b64_data;
}
/**
 * Get the length of an open file
 * @param fp an open FILE handle
 * @return file length if successful, else 0
 */
static int file_length( FILE *fp )
{
	int length = 0;
    int res = fseek( fp, 0, SEEK_END );
	if ( res == 0 )
	{
		long long_len = ftell( fp );
		if ( long_len > INT_MAX )
        {
			fprintf( stderr,"mvdfile: file too long: %ld", long_len );
            length = res = 0;
        }
		else
        {
            length = (int) long_len;
            if ( length != -1 )
                res = fseek( fp, 0, SEEK_SET );
            else
                res = 1;
        }
	}
	if ( res != 0 )
    {
		fprintf(stderr, "mvdfile: failed to read file. error %s\n",
            strerror(errno) );
        length = 0;
    }
	return length;
}
/**
 * Load an mvd from a file
 * @param file the path to the file form the PWD
 * @return the mvd object or NULL on failure
 */
MVD *mvdfile_load( char *file )
{
    MVD *mvd;
    //long start = epoch_time();
    FILE *fp = fopen( file, "r" );
    if ( fp != NULL )
    {
        int len = file_length( fp );
        char *data = malloc( len );
        if ( data != NULL )
        {
            int read = fread( data, 1, len, fp );
            if ( read == len )
                mvd = mvd_internalise( data, len );
            else
                fprintf(stderr,"mvdfile: failed to read %s\n",file);
            free( data );
        }
        else
            fprintf(stderr,"mvdfile: failed to allocate file buffer\n");
        fclose( fp );
    }
    //long end = epoch_time();
    //printf("time taken to load mvd: %ld microseconds\n",(end-start));
    return mvd;
}
/**
 * Write an MVD in the new MVD format to disk
 * @param mvd the MVD to save
 * @param file the file to save to
 * @param old if 1 save in the old format
 * @returns 1 if it worked else 0
 */
int mvdfile_save( MVD *mvd, char *file, int old ) 
{
    int res = 0;
    //printf("saving %s\n",file);
    //long start = epoch_time();
    //long end_mem,start_mem = get_mem_usage();
    FILE *dst = fopen( file, "w" );
    if ( dst != NULL )
    {
        int len=-1,written=0;
        char *b64_data = mvdfile_externalise( mvd, &len, old );
        if ( b64_data != NULL && len > 0 )
        {
            written = fwrite( b64_data, 1, len, dst );
            //end_mem = get_mem_usage();
            free( b64_data );
        }
        res = (len == written);
        if ( !res )
            fprintf(stderr,"mvdfile: failed to write MVD\n");
        fclose( dst );
    }
    else
        fprintf(stderr,"mvdfile: failed to open file %s\n",file);
    //long end = epoch_time();
    //printf("time taken to save mvd: %ld microseconds\n",(end-start));
    //printf("memory used in save: %ld bytes\n",(end_mem-start_mem));
    return res;
}
#ifdef MVD_TEST
/**
 * Test the mvdfile class
 * @param passed VAR param update with number of passed tests
 * @param failed VAR param update with number of failed tests
 */
void test_mvdfile( int *passed, int *failed )
{
    int res = 0;
    const char *gdata = "\0\3\0\0\0\6banana\0\0\0\11pineapple\0\1\0\4pear";
    char *vdata = "\0\5\0\1\0\1\0\0\0\1A\0\11Version 1\0\2\0\0\0\1B\0\11"
    "Version 2\0\0\0\0\0\1C\0\11Version 3\0\1\0\0\0\1D\0\11Version 4"
    "\0\1\0\0\0\1E\0\11Version 5";
    
    MVD *mvd = mvd_create();
    if ( mvd != NULL )
    {
        UChar **groups = readGroupTable( mvd, (unsigned char*)gdata, 33,0);
        if ( groups!=NULL )
        {
            int i = 0;
            /*char buf[64];
            while ( groups[i] != NULL )
                printf("group=%s\n",u_print(groups[i++],buf,64));*/
            res = readOldVersionTable( vdata, 94, 0, groups, mvd );
            if ( mvd_get_set_size(mvd)!=1 )
            {
                fprintf(stderr,"mvdfile: failed to read set size\n");
                res = 0;
            }
            if ( mvd_count_versions(mvd)!= 5 )
            {
                fprintf(stderr,"mvdfile: number of versions wrong\n");
                res = 0;
            }
            *passed += 1;
            i=0;
            while ( groups[i] != NULL )
                free( groups[i++] );
            free( groups );
        }
        else
        {
            fprintf(stderr,"mvdfile: failed to read group table\n");
            *failed += 1;
        }
        mvd_dispose( mvd );
    }
    if ( res == 1 )
        *passed += 1;
    else
        *failed += 1;
}
#endif