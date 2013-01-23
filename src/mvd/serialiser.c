#include <string.h>
#include "mvd/serialiser.h"
/**
 * Write the given int value to the byte array. MSB first 
 * (BigEndian order)
 * @param data a byte array to write to
 * @param p offset into data at which to begin writing
 * @param value the int value to write
 */
void write_int( unsigned char *data, int p, int value )
{
   int i,mask = 0xFF;
   for ( i=3;i>=0;i-- )
   {
       data[p+i] = (value & mask);
       value >>= 8;
   }
}
/**
 * Serialise a string, which may have any encoding
 * @param data the byte array to write to
 * @param p the offset within data to write to
 * @param str the value to serialise
 * @return the number of bytes including the count int consumed
 */
int write_string( unsigned char *data, int p, unsigned char *str ) 
{
   int i,len = 2;	// short length
   short slen = (short)strlen(str);
   write_short( data, p, slen );
   p += 2;
   for ( i=0;i<slen;i++ )
       data[i+p] = str[i];
   len += slen;
   return len;
}
/**
 * Write the given short value to the byte array. MSB first 
 * (BigEndian order)
 * @param data a byte array to write to
 * @param p offset into data at which to begin writing
 * @param value the short value to write
 */
void write_short( unsigned char *data, int p, short value )
{
   int i,mask = 0xFF;
   for ( i=1;i>=0;i-- )
   {
       data[p+i] = (unsigned char) (value & mask);
       value >>= 8;
   }
}
