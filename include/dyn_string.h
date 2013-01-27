/* 
 * File:   dyn_string.h
 * Author: desmond
 *
 * Created on January 18, 2013, 8:08 AM
 */

#ifndef DYN_STRING_H
#define	DYN_STRING_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dyn_string_struct dyn_string;
dyn_string *dyn_string_create();
dyn_string *dyn_string_create_from( char*str );
dyn_string *dyn_string_dispose( dyn_string *ds );
int dyn_string_concat( dyn_string *ds, char *tail );
int dyn_string_len( dyn_string *ds );
char *dyn_string_data( dyn_string *ds );
void test_dyn_string( int *passed, int *failed );

#ifdef	__cplusplus
}
#endif

#endif	/* DYN_STRING_H */

