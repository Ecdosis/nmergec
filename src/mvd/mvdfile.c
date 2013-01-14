#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
/*static int mvdfile_externalise( MVD *mvd, char *dst ) 
{
   int size = mvd_datasize( mvd );
   char *data = malloc(size);
   int nbytes = mvd_serialise( mvd, data );
   assert(nBytes==size);
   String str = Base64.encodeBytes( data, Base64.GZIP );
   if ( rb == null )
       writeToFile( dst, str );
   else
       writeToDatabase( dst.getName(), str, mvd.description, folderId, rb );
}*/
	
