#include <stdio.h>
#include <string.h>
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "serialiser.h"
#include "encoding.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/**
 * Write the given int value to the byte array. MSB first 
 * (BigEndian order)
 * @param data a byte array to write to
 * @param len the overall length of data
 * @param p offset into data at which to begin writing
 * @param value the int value to write
 */
void write_int( unsigned char *data, int len, int p, int value )
{
    if ( 4+p<=len )
    {
        int i,mask = 0xFF;
        for ( i=3;i>=0;i-- )
        {
            data[p+i] = (value & mask);
            value >>= 8;
        }
    }
    else
        fprintf(stderr,"serialiser: attempt to write int at %d beyond "
        "end of data at %d\n",p,len);
}
/**
 * Serialise an ascii string
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within data to write to
 * @param str the value to serialise in C string format
 * @return the number of bytes including the count int consumed
 */
int write_ascii_string( unsigned char *data, int len, int p, char *str ) 
{
    short slen = (short)strlen(str);
    write_short( data, len, p, slen );
    p += 2;
    int nchars = write_data( data, len, p, str, slen );
    if ( nchars==0 )
        fprintf(stderr,"encoding: failed to write string\n");
    return 2+nchars;
    
}/**
 * Serialise a string
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within data to write to
 * @param str the value to serialise in unicode UChar format
 * @param encoding the string's desired encoding
 * @return the number of bytes including the count int consumed
 */
int write_string( unsigned char *data, int len, int p, UChar *str, 
    char *encoding ) 
{
    short ulen = (short)u_strlen(str);
    int p_start = p;
    p += 2;
    int nchars = convert_to_encoding( str, ulen, &data[p], len-p, 
        encoding );
    if ( nchars==0 )
        fprintf(stderr,"encoding: failed to write string\n");
    write_short( data, len, p_start, nchars );
    return 2+nchars; 
}
/**
 * Write the given short value to the byte array. MSB first 
 * (BigEndian order)
 * @param data a byte array to write to
 * @param len the overall length of data
 * @param p offset into data at which to begin writing
 * @param value the short value to write
 */
void write_short( unsigned char *data, int len, int p, short value )
{
    if ( p+2 <= len )
    {
        int i,mask = 0xFF;
        for ( i=1;i>=0;i-- )
        {
            data[p+i] = (unsigned char) (value & mask);
            value >>= 8;
        }
    }
    else
        fprintf(stderr,"serialiser: attempt to write short at %d beyond "
            "end of data at %d\n",p,len);
}
/**
 * Write bytes from source to destination
 * @param dst the destination byte array
 * @param len the overall length of dst
 * @param p offset within data to write to
 * @param src the source byte array
 * @param src_len the length of src
 * @return number of bytes copied or 0 on failure
 */
int write_data( unsigned char *dst, int len, int p, unsigned char *src, 
    int src_len ) 
{
    int i;
    if ( p+src_len<=len )
    {
        for ( i=0;i<src_len;i++ )
            dst[p++] = src[i];
        return src_len;
    }
    else
    {
        fprintf(stderr,"serialiser: attempt to write data beyond end of dst\n");
        return 0;
    }
}