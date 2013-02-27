#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "unicode/uchar.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
    
/**
 * convert an integer into a temporary string
 * @param value the int value to stringify
 * @param buf the buffer to write to
 * @param len the number of bytes in buf
 * @return buf, with the value written into it
 */
char *itoa( int value, char *buf, int len )
{
    snprintf(buf,len,"%d",value);
    return buf;
}
UChar *u_strdup(UChar *in) 
{
    uint32_t len = u_strlen(in) + 1;
    UChar *result = malloc(sizeof(UChar) * len);
    u_memcpy(result, in, len);
    return result;
}
/**
 * Convert a utf-16 string of digits to a number
 * @param str a UTF-16 string
 * @return its value
 */
int u_atoi( UChar *str )
{
    int i,len = u_strlen( str );
    int value = 0;
    for ( i=0;i<len;i++ )
    {
        value *= 10;
        if ( str[i]>=48&&str[i]<=57 )
            value += str[i]-48;
        // ignore other ascii codes
    }
    return value;
}
/**
 * Generate a 16 byte random UTF-16 string for testing
 * @return the allocated string
 */
UChar *random_str()
{
    static UChar alphabetti[26] = {'a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p',
	'q','r','s','t','u','v','w','x','y','z'};
    UChar *str = calloc( 16, sizeof(UChar) );
    if ( str != NULL )
    {
        int i;
        for ( i=0;i<15;i++ )
            str[i] = alphabetti[rand()%26];
        str[15] = 0;
    }
    return str;
}
char *u_print( UChar *ustr, char *buf, int n )
{
    int i,len = u_strlen( ustr );
    int limit = n-1;
    for ( i=0;i<len&&i<limit;i++ )
        buf[i] = (char)ustr[i];
    buf[i] = 0;
    return buf;
}
void lowercase( char *str )
{
    int i,len = strlen(str);
    for ( i=0;i<len;i++ )
        str[i] = tolower(str[i]);
}
/**
 * Do a quick and dirty ascii to uchar conversion
 * @param str the ascii string
 * @param u_str the destination uchar string
 * @param u_len the number of uchars in u_str
 */
void ascii_to_uchar( char *str, UChar *u_str, int u_len )
{
    int slen = strlen( str );
    if ( slen+1 <= u_len )
    {
        int i;
        for ( i=0;i<slen;i++ )
            u_str[i] = (UChar)str[i];
        u_str[slen] = 0;
    }
    else
        fprintf(stderr,"utils: UChar buffer too short\n");
}
void calc_ukey( UChar *u_key, long value, int len )
{
    memset( u_key, 0, len*sizeof(UChar) );
    char *key = calloc( len, 1 );
    if ( key != NULL )
    {
        snprintf( key, len, "%lx", value );
        ascii_to_uchar( key, u_key, len );
        free( key );
    }
    else
    {
        fprintf(stderr,"mvd: failed to create ukey\n");
        u_key[0] = 0;
    }
}
/**
 * Strip quotation marks form the end of a string
 * @param str the string to strip
 */
void strip_quotes( char *str )
{
    int i = 0;
    while ( str[i] != 0 && (str[i] == '"' || str[i]=='\'') )
        i++;
    int j = strlen(str)-1;
    while ( j>=0 && (str[j] == '"' || str[j]=='\'') )
        j--;
    memcpy( str, &str[i], j-i+1 );
    str[j-i+1] = 0;
}