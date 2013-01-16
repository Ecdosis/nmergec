#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "char_buf.h"
#include "zip/zip.h"
#include "mvd/mvd.h"
/**
 * <h4>MVD file format</h4>
 * <ul><li>outer wrapper: base64 encoding</li>
 * <li>inner wrapper: zip compression</li></ul>
 * <p>Header:</p>
 * <ul><li>magic string: 8 bytes must be 0xDEADC0DE</li>
 * <li>data mask: 4 bytes 0=NONE</li>
 * <li>group-table offset: 4-byte int offset from start of file</li>
 * <li>version-table offset: 4-byte int offset from start of file</li>
 * <li>pairs-table offset: 4-byte int offset from start of file</li>
 * <li>data-table offset: 4-byte int offset from start of file</li>
 * <li>description: 2-byte int preceded utf-8 string</li></ul>
 * <li>encoding: 2-byte int preceded utf-8 string</li></ul>
 * <p>Tables:</p>
 * <ul><li>group-table: number of group-definitions: 2 byte int;<br/>
 * for each group: parent: 2-byte int (0 if a top level group); name: 
 * 2-byte int preceded utf-8 string; id implied by position in the table 
 * starting at 1</li>
 * <li>version-table: number of versions: 2-byte int; version-set size: 
 * 2-byte int;<br/>
 * for each version: group: 2 byte int; shortName: 2-byte int 
 * preceded utf-8 string; longName: 2-byte int preceded utf-8 string; 
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
#define MVD_MAGIC = "\336\255\300\336"

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
	
MVD *mvdfile_internalise( char *src )
{
    /**
	 * Read an MVD from a file or a database
	 * @param src the file to read it from
	 * @param props database property file
	 * @throws Exception
	 */
	public static MVD internalise( File src, Properties props ) throws Exception
	{
		MVD mvd = null;
		char[] data = null;
		if ( props == null )
			data = readFromFile( src );
		else
			data = readFromDatabase( src.getName(), props );
		if ( data != null && data.length != 0 )
		{
			byte[] bytes = base64Decode( new String(data) );
			mvd = parse( bytes );
		}
		else
			throw new MVDException( "data is empty");
		return mvd;
	}
}
