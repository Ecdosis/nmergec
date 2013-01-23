/*
 * Paul Hsieh hash function as used by Google et al.
 * http://www.azillionmonkeys.com/qed/hash.html
 */
#include <stdint.h>
#include <stdlib.h>
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t hsieh_hash (const char * data, int len) {
uint32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#ifdef MVD_TEST
#include <stdio.h>
#include <hashmap.h>
/**
 * Generate a 16 byte random string for testing
 * @return the allocated string
 */
static char *random_str()
{
    static char *alphabetti = "abcdefghijklmnopqrstuvwxyz";
    char *str = malloc( 16 );
    if ( str != NULL )
    {
        int i;
        for ( i=0;i<15;i++ )
            str[i] = alphabetti[rand()%26];
        str[15] = 0;
        return str;
    }
    return "banana";
}
/**
 * Test the hash function by generating lots of random strings 
 * and hashing them
 * @param passed VAR param update number of passed tests
 * @param failed VAR param update unber of failed tests
 * @return 1 if everything worked
 */
int test_hsieh( int *passed, int *failed )
{
    int i;
    int non_unique = 0;
    hashmap *hm = hashmap_create( 1200, 0 );
    if ( hm != NULL )
    {
        for ( i=0;i<1000;i++ )
        {
            char key[32];
            char *str = random_str();
            uint32_t hash = hsieh_hash( str, 15 );
            snprintf( key, 32, "%u", hash );
            if ( !hashmap_contains(hm,key) )
            {
                hashmap_put( hm, key, str );
                free( str );
            }
            else
                non_unique++;
        }
        if ( non_unique > 3 )
        {
            *failed += 1;
            fprintf(stderr,"hsieh: too many (>3) non_unique values: %d\n",
                non_unique);
        }
        else
            *passed += 1;
        hashmap_dispose( hm );
    }
    else
    {
        *failed += 1;
        fprintf(stderr,"hsieh: failed to allocate test hashmap\n");
    }
    return *failed == 0;
}
#endif