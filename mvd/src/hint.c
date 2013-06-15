#include <stdlib.h>
#include <stdio.h>
#include "unicode/uchar.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "vgnode.h"
#include "hint.h"
struct hint_struct
{
    bitset *bs;
    vgnode *n;
    hint *next;
    hint *prev;
};
/**
 * Create a hint object
 * @param versions the versions to clone
 * @return a finished hint or NULL
 */
hint *hint_create( bitset *versions, vgnode *n )
{
    hint *h = calloc( 1, sizeof(hint) );
    if ( h != NULL )
    {
        h->n = n;
        h->bs = bitset_clone( versions );
        if ( h->bs == NULL )
        {
            hint_dispose( h );
            h = NULL;
        }
    }
    else
        fprintf(stderr,"hint: failed to crate object\n");
    return h;
}
/**
 * Deallocate the hint
 * @param h the hint to dispose
 */
void hint_dispose( hint *h )
{
    if ( h->bs != NULL )
        bitset_dispose( h->bs );
    free( h );
}
/**
 * Remove some versions from the hint
 * @param h the hint in question
 * @param other the other set of versions to eliminate where they intersect
 * @return 1 if the hint is now empty, else 0
 */
int hint_subtract( hint *h, bitset *other )
{
    bitset_and_not( h->bs, other );
    return bitset_empty( h->bs );
}
/**
 * Does this hint
 * @param h the head of the list
 * @param bs the set of versions to seek
 * @return the hint that contain it or NULL
 */
hint *hint_contains( hint *h, bitset *bs )
{
    hint *temp = h;
    while ( temp != NULL )
    {
        if ( bitset_intersects(bs,temp->bs) )
            return temp;
        temp = temp->next;
    }
    return NULL;
}
/**
 * Cut a hint from the list
 * @param h the hint to delist
 * @return the new list head
 */
hint *hint_delist( hint *h )
{
    if ( h->next != NULL )
        h->next->prev = h->prev;
    if ( h->prev != NULL )
        h->prev->next = h->next;
    // compute new head
    hint *temp;
    if ( h->prev != NULL )
        temp = h->prev;
    else
        temp = h->next;
    if ( temp != NULL )
    {
        while ( temp->prev != NULL )
            temp = temp->prev;
    }
    return temp;
}
/**
 * Add a hint to the list
 * @param h the hint at the list head
 * @param other the hint to add
 */
void hint_append( hint *h, hint *other )
{
    while ( h->next != NULL )
        h = h->next;
    h->next = other;
    other->prev = h;
}
/**
 * Create a hint vgnode
 * @param h the hint
 * @return its vgnode
 */
vgnode *hint_node( hint *h )
{
    return h->n;
}
/**
 * OR a set of versions onto ours
 * @param h the hint
 * @param bs the versions to add
 */
void hint_or( hint *h, bitset *bs )
{
    // may reallocate h->bs
    h->bs = bitset_or( h->bs, bs );
}