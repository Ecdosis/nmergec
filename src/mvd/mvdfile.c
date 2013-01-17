#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include "char_buf.h"
#include "zip/zip.h"
#include "mvd/mvd.h"
#include "dyn_array.h"
#define MVD_MAGIC_OLD 0xDEADCODE
#define MVD_MAGIC_NEW 0x600DCODE
/**
 * <h4>MVD file format</h4>
 * <ul><li>outer wrapper: base64 encoding</li>
 * <li>inner wrapper: zip compression</li></ul>
 * <p>Header:</p>
 * <ul><li>magic string: 8 bytes must be 0xDEADC0DE (old) or 0x600DCODE (new)</li>
 * <li>data mask: 4 bytes 0=NONE (omitted in new)</li>
 * <li>group-table offset: 4-byte int offset from start of file(omitted in new)</li>
 * <li>version-table offset: 4-byte int offset from start of file</li>
 * <li>pairs-table offset: 4-byte int offset from start of file</li>
 * <li>data-table offset: 4-byte int offset from start of file</li>
 * <li>description: 2-byte int preceded utf-8 string</li></ul>
 * <li>encoding: 2-byte int preceded utf-8 string</li></ul>
 * <p>Tables:</p>
 * <ul><li>group-table (omitted in new): number of group-definitions: 
 * 2 byte int;<br> for each group: parent: 2-byte int (0 if a top level group); 
 * name: 2-byte int preceded utf-8 string; id implied by position in the table 
 * starting at 1</li>
 * <li>version-table (old): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: group: 2 byte int; shortName: 2-byte int 
 * preceded utf-8 string; longName: 2-byte int preceded utf-8 string; 
 * version numbers implied by position in table starting at 1</li>
 * <li>version-table (new): number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: versionID: 2-byte int preceded utf-8 string;  
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
 * Save an MVD to a file
 * @param mvd the MVD to save
 * @param dst the file to save it to
 * @return 1 if it worked
 */
static int mvdfile_externalise( MVD *mvd, char *dst ) 
{
    int res = 0;
    int size = mvd_datasize( mvd );
    unsigned char *data = malloc( size );
    if ( data != NULL )
    {
        int nbytes = mvd_serialise( mvd, data );
        assert(nbytes==size);
        // need to ZIP encode it first
        size_t encoded_size = b64_encode_buflen( (size_t)nbytes );
        char *encoded = malloc( encoded_size );
        if ( encoded != NULL )
        {
            b64_encode( data, size, encoded, encoded_size ); 
            FILE *out = fopen( dst, "w" );
            if ( out != NULL )
            {
                size_t nitems = fwrite( encoded, 1, encoded_size, out );
                if ( nitems != encoded_size )
                    fprintf(stderr,
                        "mvdfile: length mismatch. desired=%d, actual=%d\n",
                        (int)encoded_size,(int)nitems);
                else
                    res = 1;
                fclose( out );
            }
            else
                fprintf(stderr,"mvdfile: failed to open %s\n",dst);
            free( encoded );
        }
        else
            fprintf(stderr,"mvdfile: failed to allocate b64 buf\n");
        free( data );
    }
    else
        fprintf(stderr,"mvdfile: failed to allocate mvd data\n");
    return res;
}
/**
 * Extract the MVD magic code at the start of every MVD file
 * @param mvd_data the uncompressed mvd data structure
 * @return the mvd_magic value
 */
static unsigned mvd_magic( unsigned char *mvd_data )
{
    return *((unsigned *)mvd_data);
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
   int x = 0;
   if ( p+3 < len )
   {
       for ( int i=p;i<p+4;i++ )
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
 * @return the int read from data
 */
static short readShort( unsigned char *data, int len, int p ) 
{
   short x = 0;
   if ( p+1 < len )
   {
       for ( int i=p;i<p+2;i++ )
       {
           x <<= 8;
           x = data[i];
       }
   }
   return x;
}
/**
 * Read a 2-byte preceded UTF-8 string from an array of data bytes
 * @param data the data to read from
 * @param p the offset of the string start
 * @return an allocated string or NULL
 */
static unsigned char *readUtf8String( unsigned char *data, int len, int p ) 
{
    short slen = readShort( data, len, p );
    p += 2;
    unsigned char *str = malloc(slen+1);
    if ( str != NULL )
    {
        for ( int i=0;i<len;i++ )
        {
            str[i] = data[p+i];
        }
    }
    return str;
}
/**
 * Read the group table for an MVD from a byte array
 * @param data the byte array containing the group definitions
 * @param len the length of the mvd_data
 * @param p the start offset of the groups within data
 * @param return an allocated array of group paths or NULL
 */
static char **readGroupTable( unsigned char *mvd_data, int len, int p )
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
        char **paths = malloc(nGroups,sizeof(char*));
        if ( paths != NULL )
        {
            for ( short i=0;i<nGroups;i++ )
            {
                short parent = readShort( mvd_data, p );
                p += 2;
                short len = readShort( mvd_data, p );
                unsigned char *name = readUtf8String( mvd_data, len, p );
                p += 2 + len;
                paths
            }
        }
        return paths;
    }
}
/**
 * Parse an old format MVD
 * @param mvd_data the mvd_data to parse
 * @param len t=its length
 */
MVD *mvd_parse_old( unsigned char *mvd_data, int len )
{
    MVD mvd = NULL;
    // point after magic
    int p = 4;
    // mask type - redundant
    int maskType = readInt( mvd_data, len, p );
    p += 4;
    int groupTableOffset = readInt( mvd_data, len, p );
    p += 4;
    int versionTableOffset = readInt( mvd_data, len, p );
    p += 4;
    int pairsTableOffset = readInt( mvd_data, len, p );
    p += 4;
    int dataTableOffset = readInt( mvd_data, len, p );
    p += 4;
    short strLen = readShort( mvd_data, len, p );
    unsigned char *description = readUtf8String( mvd_data, len, p );
    p += strLen + 2;
    strLen = readShort( mvd_data, len, p );
    unsigned char *encoding = readUtf8String( mvd_data, len, p );
    mvd = mvd_create( description, encoding );
    free( description );
    free( encoding );
    p = groupTableOffset;
    readGroupTable( mvd_data, p, len, mvd );
    p = versionTableOffset;
    readVersionTable( mvd_data, p, len, mvd );
    p = pairsTableOffset;
    readPairsTable( mvd_data, p, dataTableOffset, mvd );
    return mvd;
}
MVD *mvd_parse_new( unsigned char *data, int len )
{
    return NULL;
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
    MVD *mvd = NULL;
    size_t zip_len = b64_decode_buflen( (size_t)len );
    if ( zip_len > 0 )
    {
        unsigned char *zip_data = b64_decode( data, len, zip_data, zip_len ); 
        if ( zip_data != NULL )
        {
            char_buf *buf = char_buf_create( zip_len*2 );
            if ( buf != NULL )
            {
                int res = zip_deflate( zip_data, zip_len, buf );
                if ( res )
                {
                    unsigned char *mvd_data = char_buf_get( buf, &mvd_len );
                    if ( mvd_magic(mvd_data)==MVD_MAGIC_OLD )
                        mvd = mvd_parse_old( mvd_data, mvd_len );
                    else if ( mvd_magic(mvd_data)==MVD_MAGIC_NEW )
                        mvd = mvd_parse_new( mvd_data, mvd_len );
                    else
                        fprintf(stderr,"mvdfile: mvd magic invalid\n");
                }
                else
                    fprintf(stderr,"mvdfile: zip deflate failed\n");
            }
        }
    }
    return mvd;
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
MVD *mvd_load( char *file )
{
    MVD *mvd;
    FILE *fp = fopen( file );
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
    return mvd;
}