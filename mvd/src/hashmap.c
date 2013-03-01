#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "hashmap.h"
#include "hsieh.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif


#define INITIAL_BUCKETS 12
#define MAX_RATIO 1.2f

struct hashmap_struct
{
	struct hm_bucket **buckets;
	int num_buckets;
	int num_keys;
    int use_ints;
};
struct hm_bucket
{
	UChar *key;
	void *value;
	struct hm_bucket *next;
};
/**
 * Create a fixed string-string hashmap
 * @param initial_size the size of the map to start with. 0 means default
 * @param int_keys if 1 then don't hash keys, assume they are integers
 * @return an allocated hashmap structure
 */
hashmap *hashmap_create( int initial_size, int int_keys )
{
	hashmap *map = calloc( 1,sizeof(hashmap) );
    if ( initial_size <= INITIAL_BUCKETS )
        initial_size = INITIAL_BUCKETS;
	if ( map != NULL )
	{
        map->use_ints = int_keys;
		map->buckets = calloc( initial_size, sizeof(struct hm_bucket*) );
		if ( map->buckets != NULL )
		{
			map->num_buckets = initial_size;
			map->num_keys = 0;
		}
		else
		{
			free( map );
			map = NULL;
			fprintf(stderr,"hashmap: failed to allocate hashmap buckets\n");
		}
	}
	else
		fprintf(stderr,"hashmap: couldn't allocate hashmap\n");
	return map;
}
/**
 * Delete a hashmap and everything it owns
 * @param map the doomed map
 * @param func the dispose function for values or NULL
 */
void hashmap_dispose( hashmap *map, dispose_func func )
{
    int i,j;
    for ( i=0;i<map->num_buckets;i++ )
    {
        if ( map->buckets[i] != NULL )
        {
            struct hm_bucket *hm = map->buckets[i];
            while ( hm != NULL )
            {
                struct hm_bucket *next = hm->next;
                // this fails when memwatch is turned on
                if ( func != NULL )
#ifdef MEMWATCH
                    if ( func == free )
                        free( hm->value );
                    else
                        (func)( hm->value );
#else
                    (func)(hm->value);
#endif
                free( hm->key );
                free( hm );
                hm = next;
            }
            map->buckets[i] = NULL;
        }
    }
    free( map->buckets );
    free( map );
}
/**
 * Find the bucket corresponding to the given key
 * @param map the hashmap
 * @param key the key to get the index of
 * @return the index of that key (whether or not it exists)
 */
static int key_to_bucket( hashmap *map, UChar *key )
{
    int keylen = u_strlen(key);
    uint32_t hashval = (map->use_ints)?(uint32_t)u_atoi(key)
        :hsieh_hash((char*)key,keylen*sizeof(UChar));
    return hashval % map->num_buckets;
}
/**
 * Set the key, value pair in a bucket
 * @param b the bucket
 * @param key the key for it
 * @param value the value that goes with the key
 */
static void hashmap_bucket_set( struct hm_bucket *b, UChar *key, void *value )
{
	b->key = u_strdup( key );
	if ( b->key == NULL )
		fprintf(stderr,"hashmap: failed to reallocate key\n");
    else
    {
        b->value = value;
        b->next = NULL;
    }
}
/**
 * Reallocate all the keys in a new bucket set that must be
 * 1.5 times bigger than before
 * @param map the map to rehash
 * @return 1 if it worked, else 0
 */
static int hashmap_rehash( hashmap *map )
{
	int i,old_size = map->num_buckets;
	int num_buckets = old_size + (old_size/2);
	struct hm_bucket **old_buckets = map->buckets;
	map->buckets = calloc( num_buckets, sizeof(struct hm_bucket*) );
    //printf("resizing\n");
	if ( map->buckets == NULL )
    {
		fprintf(stderr,"hashmap: failed to resize hashmap\n");
        return 0;
    }
    else
    {
        // copy the old keys over
        map->num_keys = 0;
        map->num_buckets = num_buckets;
        for ( i=0;i<old_size;i++ )
        {
            struct hm_bucket *b = old_buckets[i];
            while ( b != NULL )
            {
                struct hm_bucket *old = b;
                hashmap_put( map, b->key, b->value );
                b = b->next;
                //printf("freeing %lx\n",(long)old->key);
                free( old->key );
                free( old );
            }
        }
        free( old_buckets );
        return 1;
    }
}
/**
 * Put a key into the hashmap or test for membership. The
 * destination bucket is decided by modding the hash by
 * the number of bucket slots available. This gives us up
 * to 4 billion buckets maximum.
 * @param map the hashmap to add to
 * @param key the key to put in there
 * @param value the value at this key
 * @return 1 if it was added, 0 otherwise (already there)
 */
int hashmap_put( hashmap *map, UChar *key, void *value )
{
    if ( key==NULL||value==NULL )
    {
        fprintf(stderr,"hashmap: key or value is NULL.\n");
        return 0;
    }
	int bucket = key_to_bucket( map, key );
    if ( map->buckets[bucket] == NULL )
	{
		map->buckets[bucket] = calloc( 1,sizeof(struct hm_bucket) );
		if ( map->buckets[bucket] == NULL )
        {
			fprintf(stderr,"hashmap: failed to allocate store for bucket\n");
            return 0;
        }
        else
        {
            hashmap_bucket_set( map->buckets[bucket], key, value );
            map->num_keys++;
            return 1;
        }
	}
	else if ( (float)map->num_keys/(float)map->num_buckets > MAX_RATIO )
	{
		if ( hashmap_rehash(map) )
            return hashmap_put( map, key, value );
        else
            return 0;
	}
	else
	{
		struct hm_bucket *b = map->buckets[bucket];
        while ( b->next != NULL )
		{
			// if key already present, just return
			if ( u_strcmp(key,b->key)==0 )
				return 0;
			else
				b = b->next;
		}
		// key not found
		b->next = calloc( 1,sizeof(struct hm_bucket) );
		if ( b->next == NULL )
        {
			fprintf(stderr,"hashmap: failed to allocate store for bucket\n");
            return 0;
        }
        else
        {
            hashmap_bucket_set( b->next, key, value );
            map->num_keys++;
            return 1;
        }
	}
}
/**
 * Get the value for a key
 * @param map the hashmap to query
 * @param key the key to test for
 * @return the key's value or NULL if not found
 */
void *hashmap_get( hashmap *map, UChar *key )
{
	int bucket = key_to_bucket( map, key );
	struct hm_bucket *b = map->buckets[bucket];
	while ( b != NULL )
	{
		// if key already present, just return
		if ( u_strcmp(key,b->key)==0 )
			return b->value;
		else
			b = b->next;
	}
	// not present
	return NULL;
}
/**
 * Does the hashmap contain a key?
 * @param map the hashmap to query
 * @param key the key to test for
 * @return 1 if present, 0 otherwise
 */
int hashmap_contains( hashmap *map, UChar *key )
{
	int bucket = key_to_bucket( map, key );
	struct hm_bucket *b = map->buckets[bucket];
    while ( b != NULL )
    {
        // if key already present, just return
        if ( u_strcmp(key,b->key)==0 )
            return 1;
        else
            b = b->next;
    }
	return 0;
}
/**
 * Get the size of this hashmap
 * @param map the hashmap to get the size of
 * @return the number of its current entries
 */
int hashmap_size( hashmap *map )
{
	return map->num_keys;
}
/**
 * Remove a key from the map
 * @param hm the hashmap in question
 * @param key the key to remove
 * @param func to dispose of the value
 * @retiurn 1 if it was there and removed
 */
int hashmap_remove( hashmap *map, UChar *key, dispose_func func )
{
    int res = 0;
    int bucket = key_to_bucket( map, key );
    if ( map->buckets[bucket] != NULL )
    {
        struct hm_bucket *b = map->buckets[bucket];
        struct hm_bucket *prev = NULL;
        while ( b != NULL )
        {
            if ( u_strcmp(b->key,key)==0 )
            {
                free( b->key );
                if ( prev == NULL )
                    map->buckets[bucket] = b->next;
                else
                    prev->next = b->next;
                if ( func != NULL )
                    (func)(b->value);
                free( b );
                map->num_keys--;
                res = 1;
                break;
            }
            prev = b;
            b = b->next;
        }
    }
    return res;
}
/**
 * Get the keys of this hashmap as an array
 * @param map the map in question
 * @param array an array just big enough for the keys
 */
void hashmap_to_array( hashmap *map, UChar **array )
{
	int i,j;
	for ( j=0,i=0;i<map->num_buckets;i++ )
	{
		struct hm_bucket *b = map->buckets[i];
		while ( b != NULL )
		{
			array[j++] = b->key;
			b = b->next;
		}
	}
}
/**
 * is this map empty?
 * @param map the map in question
 * @return 1 if it is else 0
 */
int hashmap_is_empty( hashmap *map )
{
    return map->num_keys==0;
}
/**
 * Clear all values from a map
 * @param map the map to clear
 */
void hashmap_clear( hashmap *map )
{
    int i;
    for ( i=0;i<map->num_buckets;i++ )
    {
        if ( map->buckets[i] != NULL )
        {
            struct hm_bucket *hm = map->buckets[i];
            while ( hm != NULL )
            {
                struct hm_bucket *next = hm->next;
                free( hm->key );
                free( hm );
                hm = next;
            }
            map->buckets[i] = NULL;
        }
    }
    map->num_keys = 0;
}
#ifdef MVD_TEST
/**
 * Test the hashmap
 * @param passed VAR param update number of passed tests
 * @param failed VAR param update number of failed tests
 */
void test_hashmap( int *passed, int *failed )
{
    hashmap *hm = hashmap_create( 200, 0 );
    int total = 0;
    if ( hm != NULL )
    {
        int i;
        for ( i=0;i<10000;i++ )
        {
            UChar *key = random_str();
            UChar *value = random_str();
            if ( !hashmap_contains(hm,key) )
            {
                if ( hashmap_put(hm,key,value) )
                    total++;
            }
            if ( u_strcmp(key,(UChar*)"\x0062\x0061\x006E\x0061\x006E\x0061")!=0 )
                free( key );
        }
        if ( total == hashmap_size(hm) )
            *passed += 1;
        else
            *failed += 1;
        // test to_array and delete values
        UChar **array = calloc( hashmap_size(hm), sizeof(char*) );
        if ( array != NULL )
        {
            hashmap_to_array( hm, array );
            for ( i=0;i<total;i++ )
            {
                char *value = hashmap_get( hm, array[i] );
                if ( value != NULL )
                    free( value );
                else
                    break;
            }
            if ( i==total )
                *passed += 1;
            else
            {
                *failed += 1;
                fprintf(stderr,"hashmap: couldn't find value for key %d\n",i);
            }
            free( array );
        }
        // test clear
        hashmap_clear( hm );
        if ( hashmap_size(hm)==0 )
            *passed += 1;
        else
        {
            *failed += 1;
            fprintf(stderr,"hashmap: hashmap not empty after clear\n");
        }
        hashmap_dispose( hm, NULL );
    }
}
#endif