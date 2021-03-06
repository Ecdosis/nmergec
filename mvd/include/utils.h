/* 
 * File:   utils.h
 * Author: desmond
 *
 * Created on January 22, 2013, 1:57 PM
 */

#ifndef UTILS_H
#define	UTILS_H

#ifdef	__cplusplus
extern "C" {
#endif
char *itoa( int value, char *buf, int len );
UChar *u_strdup(UChar *in);
UChar *u_strndup(UChar *in, int len);
int u_atoi( UChar *str );
UChar *random_str();
char *u_print( UChar *ustr, char *buf, int n );
void lowercase( char *str );
void ascii_to_uchar( char *str, UChar *u_str, int len );
void calc_ukey( UChar *u_key, long value, int len );
void strip_quotes( char *str );
hashmap *parse_options( char *options );
int file_size( const char *file_name );
char *read_file( char *file, int *len );
#ifdef MVD_TEST
void test_utils( int *passed, int *failed );
#endif
#ifdef __LITTLE_ENDIAN__ 
#define SLASH (UChar*)"\x2F\x00"
#elif  __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SLASH (UChar*)"\x2F\x00"
#else
#define SLASH (UChar*)"\x00\x2F"
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */

