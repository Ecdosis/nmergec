#include <stdlib.h>
#include <stdio.h>
#include "bitset.h"
#ifdef MVD_TEST 
#include "memwatch.h"
#endif
struct bitset_struct
{
    unsigned char *data;
    int allocated;
    
};

/**
 * Create a new bitset
 * @return the finished bitset or NULL if it failed (unlikely)
 */
bitset *bitset_create()
{
    bitset *bs = calloc( 1, sizeof(bitset) );
    if ( bs != NULL )
    {
        bs->data = calloc( 4,sizeof(unsigned char) );
        if ( bs->data == NULL )
        {
            fprintf(stderr,"bitset: failed to allocate data\n");
            free( bs );
            bs = NULL;
        }
        else
            bs->allocated = 4;
    }
    else
        fprintf(stderr,"bitset: failed to allocate object\n");
    return bs;
}
/**
 * Dispose of a bitset
 * @param bs the bitset to dispose
 */
void bitset_dispose( bitset *bs )
{
    if ( bs->data != NULL )
        free( bs->data );
    free( bs );
}
/**
 * Resize a bitset if possible
 * @param bs the bitset in question
 * @param required the number of bytes needed in data
 * @return 1 if it worked else 0
 */
static int bitset_resize( bitset *bs, int required )
{
    int res = 1;
    int i,mod2 = required % 4;
    if ( mod2 > 0 )
        required += 4-mod2;
    unsigned char *temp = calloc(required,1); 
    if ( temp != NULL )
    {
        for ( i=0;i<bs->allocated;i++ )
            temp[i] = bs->data[i];
        free( bs->data );
        bs->data = temp;
        bs->allocated = required;
    }
    else
    {
        fprintf(stderr,"bitset: failed to reallocate data\n");
        res = 0;
    }
    return res;
}
/**
 * Set an individual bit
 * @param bs the bitset in question
 * @param i the index of the bit (zero-based)
 * @return 1 if it worked or 0 if it needed to reallocate and failed
 */
int bitset_set( bitset *bs, int i )
{
    int index = i/8;
    int mod = i%8;
    int res = 1;
    if ( index+1 > bs->allocated )
        res = bitset_resize(bs,index+1);
    if ( res )
    {
        unsigned char bit = 128;
        bs->data[index] |= bit>>mod;
    }
    return res;
}
/**
 * Get a particular bit (1 or 0) at the given index
 * @param bs the bitset in question
 * @param index the zero-based index 
 * @return 1 if the bit was set else 0
 */
int bitset_get( bitset *bs, int index )
{
    int i = index/8;
    int mod = index%8;
    unsigned char mask = 128;
    return bs->data[i] & mask>>mod;
}
/**
 * The simple maximum
 * @param a the first integer
 * @param b another integer
 * @return the maximum of the two
 */
static int max( int a, int b )
{
    return (a>b)?a:b;
}
/**
 * The simple minimum
 * @param a the first integer
 * @param b another integer
 * @return the minimum of the two
 */
static int min( int a, int b )
{
    return (a>b)?a:b;
}/**
 * OR two bitsets together
 * @param bs this bitset which will be modified
 * @param other the other bitset which will not
 * @return 1 if the operation succeeded, 0 if this bitset could not be resized
 */
int bitset_or( bitset *bs, bitset *other )
{
    int required = max(other->allocated,bs->allocated);
    int res = bitset_resize( bs, required );
    if ( res )
    {
        int i;
        for ( i=0;i<required;i++ )
            bs->data[i] |= other->data[i];
    }
    return res;
}
/**
 * And two bitsets together
 * @param bs the bitset that will be changed
 * @param other another bitset that will not
 */
void bitset_and( bitset *bs, bitset *other )
{
    int i;
    int min_index = min(bs->allocated,other->allocated);
    for ( i=0;i<min_index;i++ )
        bs->data[i] &= other->data[i];
}
/**
 * How many bits have been set?
 * @param bs the bitset in question
 * @return the bitset's cardinality
 */
int bitset_cardinality( bitset *bs )
{
    int i;
    int count = 0;
    for ( i=0;i<bs->allocated;i++ )
    {
        int j;
        unsigned char mask = 128;
        for ( j=0;j<8;j++ )
        {
            if ( bs->data[i] & mask )
                count++;
            mask >>= 1;
        }
    }
    return count;
}
/**
 * Print a bitset to stdout
 * @param bs the bitset to print
 */
static void bitset_print( bitset *bs )
{
    int i,j;
    for ( i=0;i<bs->allocated;i++ )
    {
        unsigned char mask = 128;
        for ( j=0;j<8;j++ )
        {
            if ( bs->data[i] & mask>>j )
                printf("%d",1);
            else
                printf("%d",0);
        }
    }
    printf("\n");
}
/**
 * Test this object
 * @param passed VAR param update number of passed tests
 * @param faield VAR param update number of failed tests
 * @return 1 if it all worked
 */
int bitset_test( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    bitset *other = bitset_create();
    if ( bs == NULL || other == NULL )
        *failed += 1;
    else
    {
        *passed += 1;
        bitset_set( bs, 0 );
        bitset_set( bs, 4 );
        if ( bitset_cardinality(bs) == 2 )
        {
            *passed += 1;
            bitset_set( bs, 32 );
            bitset_set( bs, 63 );
            if ( bs->allocated==8 )
                *passed += 1;
            else
            {
                *failed += 1;
                fprintf(stderr,"bitset: wrong number of data bytes %d\n",
                    bs->allocated);
            }
            //bitset_print(bs);
            int card = bitset_cardinality(bs);
            if ( card==4 )
                *passed += 1;
            else
                fprintf(stderr,"bitset: cardinality wrong %d\n",card);
            bitset_set( other, 4 );
            bitset_set( other, 32 );
            bitset_and( bs, other );
            card = bitset_cardinality(bs);
            if ( card==2 )
                *passed += 1;
            else
                fprintf(stderr,"bitset: cardinality wrong %d\n",card);
            bitset_set( other, 39 );
            bitset_set( other, 22 );
            bitset_or( bs, other );
            card = bitset_cardinality(bs);
            if ( card==4 )
                *passed += 1;
            else
                fprintf(stderr,"bitset: cardinality wrong %d\n",card);
            if ( bitset_get( bs, 39) )
                *passed += 1;
            else
            {
                fprintf(stderr,"bitset: failed bitset_get\n");
                *failed += 1;
            }
        }
        else
        {
            fprintf(stderr,
                "bitset: failed to set bits or cardinality wrong\n");
            *failed += 1;
            
        }
        // if this fails memwatch will pick it up
        bitset_dispose( bs );
        bitset_dispose( other );
    }
}
#ifdef BITSET_DEBUG
int main( int argc, char **argv )
{
    int passed = 0;
    int failed = 0;
    bitset_test( &passed, &failed );
    printf("passed %d failed %d tests\n",passed,failed);
}
#endif
