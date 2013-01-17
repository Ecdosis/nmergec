#include <stdlib.h>
#include "bitset.h"
#include "mvd/pair.h"
struct pair_struct
{
    bitset *versions;
    unsigned char *data;
    int len;
};
/**
 * Dispose of a singel pair
 * @param p the pair in question
 */
void pair_dispose( pair *p )
{
    bitset_dispose( p->versions );
    if ( p->data != NULL )
        free( p->data );
}