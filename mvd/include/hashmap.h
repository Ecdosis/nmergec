/* 
 * File:   hashmap.h
 * Author: desmond
 *
 * Created on 19 Jan 2013, 4:57 AM
 */

#ifndef HASHMAP_H
#define	HASHMAP_H

typedef struct hashmap_struct hashmap;
typedef void (*dispose_func)(void*);
hashmap *hashmap_create( int initial_size, int int_keys );
void hashmap_dispose( hashmap *map, dispose_func func );
int hashmap_put( hashmap *map, UChar *key, void *value );
void *hashmap_get( hashmap *map, UChar *key );
int hashmap_contains( hashmap *map, UChar *key );
int hashmap_size( hashmap *map );
int hashmap_is_empty( hashmap *map );
void hashmap_to_array( hashmap *map, UChar **array );
int hashmap_remove( hashmap *map, UChar *key, dispose_func func );
void hashmap_clear( hashmap *map );
#ifdef MVD_TEST
void test_hashmap( int *passed, int *failed );
#endif
#endif	/* HASHMAP_H */
