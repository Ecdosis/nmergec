/* 
 * File:   adder.h
 * Author: desmond
 *
 * Created on 14 April 2014, 1:42 PM
 */

#ifndef ADDER_H
#define	ADDER_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct adder_struct adder;
adder *adder_create( plugin_log *log );
void adder_dispose( adder *add );
UChar *convert_to_unicode( char *src, int src_len, char *encoding, 
    int *dst_len, plugin_log *log );
int adder_set_options( adder *add, hashmap *map );
UChar *adder_vid( adder *a );
UChar *adder_mvd_desc( adder *a );
void adder_set_mvd_desc( adder *a, UChar *desc );
UChar *adder_version_desc( adder *a );
char *adder_encoding( adder *a );

#ifdef	__cplusplus
}
#endif

#endif	/* ADDER_H */

