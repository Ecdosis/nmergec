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
 * Serialise a simple ascii string
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
 * Serialise a unicode string in a given encoding
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
    int nchars = convert_to_encoding( str, ulen, &data[p], len-2, encoding );
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
#ifdef MVD_TEST
#include <stdlib.h>
#include <limits.h>
#include <time.h>
static UChar ustr[] = {'o','v','e','r','l','a','y',0};
static short read_short( unsigned char *data, int *p )
{
    int i;
    short value = 0;
    for ( i=0;i<2;i++ )
    {
        value += data[(*p)+i];
        if ( i < 1 )
            value <<= 8;
    }
    *p += 2;
    return value;
}
static int read_int( unsigned char *data, int *p )
{
    int i,value = 0;
    for ( i=0;i<4;i++ )
    {
        value += data[(*p)+i];
        if ( i < 3 )
            value <<= 8;
    }
    *p += 4;
    return value;
}
void test_serialiser( int *passed, int *failed )
{
    unsigned char data[128];
    int int_val = (int)(time(NULL)%INT_MAX);
    write_int( data, 128, 0, int_val );
    int p = 0;
    int value = read_int( data, &p );
    if ( value != int_val )
    {
        fprintf(stderr,"serialiser: int was %d should be %d\n",value,99);
        (*failed)++;
    }
    else
        (*passed)++;
    short short_val = (short)(time(NULL)%SHRT_MAX);
    write_short( data, 128-p, p, short_val );
    short svalue = read_short( data, &p );
    if ( svalue != short_val )
    {
        fprintf(stderr,"serialiser: short was %d should be %d\n",svalue,27);
        (*failed)++;
    }
    else
        (*passed)++;
    int len = write_ascii_string( data, 128-p, p, "banana" );
    if ( len != 8 || strncmp(&data[p+2],"banana",6)!=0 )
    {
        fprintf(stderr,"serialiser: ascii string incorrect\n");
        (*failed)++;
    }
    else
    {
        p += len;
        (*passed)++;
    }
    len = write_string( data, 128-p, p, ustr, "utf-8" );
    int newlen=0;
    if ( len > 0 )
    {
        int dstlen = u_strlen(ustr)+1;
        UChar *dst = calloc( dstlen, sizeof(UChar) );
        if ( dst != NULL )
        {
            newlen = convert_from_encoding( &data[p+2], len-2, dst, dstlen, 
                "utf-8" );
            if ( u_strcmp(dst,ustr)!=0 )
            {
                (*failed)++;
                fprintf(stderr,"serialiser: failed to read back string\n");
            }
            else
                (*passed)++;
            free( dst );
        }
        else
        {
            fprintf(stderr,"serialiser: failed to copy string\n");
            (*failed)++;
        }
        p += len;
    }
    else
    {
        fprintf(stderr,"serialiser: failed to write string\n");
        (*failed)++;
    }
    len = write_data( data, 128-p, p, "banana", 6 ); 
    if ( len != 6 || strncmp(&data[p],"banana",6)!=0 )
    {
        (*failed)++;
        fprintf(stderr,"serialiser: failed to write raw data\n");
    }
    else
        (*passed)++;
}
#endif