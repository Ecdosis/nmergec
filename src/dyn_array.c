#include <stdlib.h>
#include <stdio.h>
#include "dyn_array.h"

struct dyn_array_struct
{
    int allocated;
    void **items;
    int pos;
};

/**
 * Create  adynamic array
 * @param initial_size the number of elements to start with
 * @return the allocated dynamic array
 */
dyn_array *dyn_array_create( int initial_size )
{
    dyn_array *da = calloc( 1, sizeof(dyn_array) );
    if ( da != NULL )
    {
        da->items = calloc(initial_size,sizeof(void*));
        if ( da->items != NULL )
        {
            da->allocated = initial_size;
        }
        else
        {
            fprintf(stderr,"dyn_array: failed to allocate items\n");
            free( da );
            da = NULL;
        }
    }
    return da;
}
/**
 * Dispose of a dynamic array
 * @param da the dynamic array in question
 */
void dyn_array_dispose( dyn_array *da )
{
    if ( da->items != NULL )
        free( da->items );
    free( da );
}
/**
 * Dispose of a dynamic array
 * @param da the dynamic array in question
 */
int dyn_array_size( dyn_array *da )
{
    return da->pos;
}
/**
 * Get an item from the array
 * @param da the dynamic array in question
 * @param index the index of the desired item
 * @return the item (cast to desired type)
 */
void *dyn_array_get( dyn_array *da, int index )
{
    return da->items[index];
}
/**
 * Append an item to the array
 * @param da the dynamic array in question
 * @param obj the item to store
 * @return 1 if it worked else 0
 */
int dyn_array_add( dyn_array *da, void *obj )
{
    int res = 1;
    if ( da->pos+1 >= da->allocated )
    {
        int i,new_size = (da->allocated*3)/2;
        void ** temp = calloc( new_size, sizeof(void*) );
        if ( temp != NULL )
        {
            for ( i=0;i<da->pos;i++ )
                temp[i] = da->items[i];
            free( da->items );
            da->items = temp;
            da->allocated = new_size;
        }
        else
        {
            fprintf(stderr,"dyn_array: failed to reallocate items\n");
            res = 0;
        }
    }
    if ( res )
        da->items[da->pos++] = obj;
    return res;
}
#ifdef MVD_TEST
void test_dyn_array( int *passed, int *failed )
{
    dyn_array *da = dyn_array_create( 2 );
    if ( da != NULL )
    {
        int res = dyn_array_add( da, "banana" );
        if ( res )
        {
            res = dyn_array_add( da, "apple" );
            if ( res )
            {
                *passed += 1;
                res = dyn_array_add( da, "pineapple" );
                if ( !res )
                {
                    fprintf(stderr,"dyn_array: failed teo reallocate array\n");
                    *failed += 1;
                }
                else
                {
                    char *str1 = dyn_array_get( da, 0 );
                    char *str2 = dyn_array_get( da, 1 );
                    char *str3 = dyn_array_get( da, 2 );
                    if ( str1 != NULL && strcmp(str1,"banana")==0 
                        && str2 != NULL && strcmp(str2,"apple")==0
                        && str3 != NULL && strcmp(str3,"pineapple")==0 )  
                        *passed += 1;
                    else
                    {
                        fprintf(stderr,"dyn_array: failed to retrieve "
                            "strings from array\n");
                        *failed += 1;
                    }
                }
            }
            else
            {
                fprintf(stderr,"dyn_array: failed store string in array\n");
                *failed += 1;
            }
        }
        dyn_array_dispose( da );
    }
    else
    {
        fprintf(stderr,"dyn_array: failed to allocate object\n");
        *failed += 1;
    }
}
#endif