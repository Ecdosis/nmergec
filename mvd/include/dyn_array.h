/* 
 * File:   dyn_array.h
 * Author: desmond
 *
 * Created on January 16, 2013, 2:02 PM
 */

#ifndef DYN_ARRAY_H
#define	DYN_ARRAY_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dyn_array_struct dyn_array;
dyn_array *dyn_array_create( int initial_size );
void dyn_array_dispose( dyn_array *da );
int dyn_array_size( dyn_array *da );
void *dyn_array_get( dyn_array *da, int index );
int dyn_array_add( dyn_array *da, void *obj );
void **dyn_array_data( dyn_array *da );
void dyn_array_remove( dyn_array *da, int i );

#ifdef MVD_TEST
void test_dyn_array( int *passed, int *failed );
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* DYN_ARRAY_H */

