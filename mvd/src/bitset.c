#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define MIN_BITS 16
#define DATA_MINSIZE 2
// variable size bitsets
struct bitset_struct
{
    short allocated;
    unsigned char data[DATA_MINSIZE];
};
/**
 * Create a bitset of a given size
 * @param bits the maximum number of bits to represent
 * @return an allocated bitset or NULL
 */
bitset *bitset_create_exact( int bits )
{
    if ( bits < MIN_BITS )
        bits = MIN_BITS;
    size_t byteSize = bits/8;
    size_t extraBytes = byteSize-DATA_MINSIZE;
    size_t size = sizeof(bitset)+extraBytes;
    bitset *bs = calloc( size, 1 );
    if ( bs == NULL )
        fprintf(stderr,"bitset: failed to allocate object\n");
    else
        bs->allocated = byteSize;
    return bs;
}
/**
 * Create a new bitset. defaults to a 4-byte int
 * @return the finished bitset or NULL if it failed (unlikely)
 */
bitset *bitset_create()
{
    return bitset_create_exact( 32 );
}
/**
 * Make an exact copy of this bitset
 * @param bs the bitset to copy
 * @return the new copied bitset or NULL if it failed
 */
bitset *bitset_clone( bitset *bs )
{
    bitset *new_bs = bitset_create_exact( bs->allocated*8 );
    if ( new_bs != NULL )
        memcpy( new_bs->data, bs->data, bs->allocated );
    return new_bs;
}
int bitset_allocated( bitset *bs )
{
    return bs->allocated;
}
/**
 * Dispose of a bitset
 * @param bs the bitset to dispose
 */
void *bitset_dispose( bitset *bs )
{
    free( bs );
    return NULL;
}
/**
 * Resize a bitset if possible, aligned on a 4-byte boundary
 * @param bs the bitset in question
 * @param required the number of BYTES needed in data
 * @return the new bitset or NULL if it failed
 */
static bitset *bitset_resize( bitset *bs, size_t required )
{
    int i,mod2 = required % 4;
    if ( mod2 > 0 )
        required += 4-mod2;
    required += sizeof(int);
    bitset *temp = malloc( required ); 
    if ( temp != NULL )
    {
        memset( temp, 0, required );
        for ( i=0;i<bs->allocated;i++ )
            temp->data[i] = bs->data[i];
        temp->allocated = required - sizeof(int);
        free( bs );
    }
    else
    {
        fprintf(stderr,"bitset: failed to reallocate data\n");
    }
    return temp;
}
/**
 * Set an individual bit
 * @param bs the bitset in question
 * @param i the index of the bit (zero-based)
 * @return the original or reallocated bitset or NULL if it failed
 */
bitset *bitset_set( bitset *bs, int i )
{
    int index = i/8;
    int mod = i%8;
    if ( index+1 > bs->allocated )
        bs = bitset_resize(bs,index+1);
    if ( bs != NULL )
    {
        unsigned char bit = 1;
        bs->data[index] |= bit<<mod;
    }
    return bs;
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
    unsigned char mask = 1;
    return bs->data[i] & mask<<mod != 0;
}
/**
 * Get the next set bit 
 * @param bs the bitset to act on
 * @param bit the position to start from (and including)
 * @return the index of the next set bit or -1 if none found
 */
int bitset_next_set_bit( bitset *bs, int bit )
{
    int i;
    int mod = (bit%8);
    for ( i=bit/8;i<bs->allocated;i++ )
    {
        int j;
        unsigned char mask = 1<<mod;
        for ( j=mod;j<8;j++ )
        {
            if ( bs->data[i] & mask )
            {
                return i*8+j;
            }
            mask <<= 1;
        }
        mod = 0;
    }
    return -1;
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
 * @return the original or reallocated bitset or NULL if it failed
 */
bitset *bitset_or( bitset *bs, bitset *other )
{
    int required = max(other->allocated,bs->allocated);
    bs = bitset_resize( bs, required );
    if ( bs != NULL )
    {
        int i;
        for ( i=0;i<other->allocated;i++ )
            bs->data[i] |= other->data[i];
    }
    return bs;
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
 * Do two bitsets intersect?
 * @param s the first bitset
 * @param b the second bitset
 * @return 1
 */
int bitset_intersects( bitset *a, bitset *b )
{
    int i;
    int res = 0;
    for ( i=0;i<a->allocated&&i<b->allocated;i++ )
    {
        if ( (a->data[i] & b->data[i]) != 0 )
        {
            res = 1;
            break;
        }
    }
    return res;
}
/**
 * Is this bitset empty?
 * @param bs the bitset in question
 * @return 1 if it has no bits set to 1
 */
int bitset_empty( bitset *bs )
{
    int i,res=1;
    for ( i=0;i<bs->allocated;i++ )
    {
        if ( bs->data[i]!= 0 )
        {
            res = 0;
            break;
        }
    }
    return res;
}
/**
 * Get a whole byte of bitset info
 * @param bs the bitset to get it from
 * @param index the index of the byte
 * @return the byte
 */
unsigned char bitset_get_byte( bitset *bs, int index )
{
    if ( index < bs->allocated )
        return bs->data[index];
    else
        return 0;
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
        unsigned char mask = 1;
        for ( j=0;j<8;j++ )
        {
            if ( bs->data[i] & mask )
                count++;
            mask <<= 1;
        }
    }
    return count;
}
/**
 * Clear all the bits in this bitset
 * @param bs the bitset to clear
 */
void bitset_clear( bitset *bs )
{
    int i;
    for ( i=0;i<bs->allocated;i++ )
        bs->data[i] = 0;
}
/**
 * Clear those bits common to two bitsets
 * @param bs the bitset whose bits will be cleared
 * @param other the other read only bitset
 */
void bitset_and_not( bitset *bs, bitset *other )
{
    int i;
    unsigned char b;
    for ( i=0;i<bs->allocated&&i<other->allocated;i++ )
    {
        b = other->data[i];
        bs->data[i] &= ~b;
    }
}
#ifdef MVD_TEST
/**
 * Print a bitset to stdout
 * @param bs the bitset to print
 */
static void bitset_print( bitset *bs )
{
    int i,j;
    for ( i=0;i<bs->allocated;i++ )
    {
        unsigned char mask = 1;
        for ( j=0;j<8;j++ )
        {
            if ( bs->data[i] & mask<<j )
                printf("%d",1);
            else
                printf("%d",0);
        }
    }
    printf("\n");
}
static void bit_test( bitset *bs, int start, int expected, 
    int *passed, int *failed )
{
    int bit = bitset_next_set_bit( bs, start );
    if ( bit != expected )
    {
        fprintf(stderr,"bitset: next set bit not %d but %d\n",expected,bit);
        *failed += 1;
    }
    else
        *passed += 1;
}
/**
 * Test this object
 * @param passed VAR param update number of passed tests
 * @param faield VAR param update number of failed tests
 */
void test_bitset( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    bitset *other = bitset_create();
    if ( bs == NULL || other == NULL )
        *failed += 1;
    else
    {
        *passed += 1;
        bs = bitset_set( bs, 0 );
        if ( bs != NULL )
            bs = bitset_set( bs, 4 );
        if ( bs != NULL && bitset_cardinality(bs) == 2 )
        {
            *passed += 1;
            bs = bitset_set( bs, 32 );
            if ( bs != NULL )
                bs = bitset_set( bs, 63 );
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
            {
                *failed += 1;
                fprintf(stderr,"bitset: cardinality wrong (1) %d\n",card);
            }
            other = bitset_set( other, 4 );
            if ( other != NULL )
                other = bitset_set( other, 32 );
            if ( bs != NULL )
                bitset_and( bs, other );
            card = bitset_cardinality(bs);
            if ( card==2 )
                *passed += 1;
            else
            {
                *failed += 1;
                fprintf(stderr,"bitset: cardinality wrong (2) %d\n",card);
            }
            if ( other != NULL )
                other = bitset_set( other, 39 );
            if ( other != NULL )
                other = bitset_set( other, 22 );
            if ( bs != NULL )
                bs = bitset_or( bs, other );
            card = bitset_cardinality(bs);
            if ( card==4 )
                *passed += 1;
            else
            {
                fprintf(stderr,"bitset: cardinality wrong (3) %d\n",card);
                *failed += 1;
            }
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
        bitset_dispose( other );
        bitset_clear( bs );
        bs = bitset_set( bs, 6 );
        bs = bitset_set( bs, 12 );
        bs = bitset_set( bs, 23 );
        bit_test( bs, 0, 6, passed, failed );
        bit_test( bs, 7, 12, passed, failed );
        bit_test( bs, 13, 23, passed, failed );
        bit_test( bs, 24, -1, passed, failed );
        bitset_dispose( bs );
    }
}
#endif
