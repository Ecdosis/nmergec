#include <stdio.h>
#include <stdlib.h>
#include "unicode/uchar.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "dyn_array.h"
#include "linkpair.h"

#define IMPLICIT_SIZE 12
struct linkpair_struct
{
    // the original pair from the mvd pairs array
    pair *p;
    // doubly-linked list
    linkpair *left;
    linkpair *right;
    // offset in suffixtree text if aligned to new version
    int st_off;
};
linkpair *linkpair_create( pair *p, plugin_log *log )
{
    linkpair *lp = calloc( 1, sizeof(linkpair) );
    if ( lp != NULL )
        lp->p = p;
    else
        plugin_log_add( log, "linkpair: failed to create object\n");
    bitset *bs = pair_versions(p);
    if ( bitset_allocated(bs)>8 )
        printf(">8");
    return lp;
}
/**
 * Just dispose of us
 * @param lp the linkpair object to free
 */
void linkpair_dispose( linkpair *lp )
{
    free( lp );
}
/**
 * Get the amalgamated versions of a set of linkpairs
 * @param set the set of linkpairs as an array
 * @return a bitset being all their versions ORed together
 */
static bitset *get_set_versions( dyn_array *set )
{
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        int i;
        for ( i=0;i<dyn_array_size(set);i++ )
        {
            linkpair *lp = dyn_array_get(set,i);
            bs = bitset_or( bs, pair_versions(lp->p) );
        }
    }
    return bs;
}
/**
 * Set the left pointer
 * @param lp the linkpair to set it for
 * @param left the next pair on the left
 */
void linkpair_set_left( linkpair *lp, linkpair *left )
{
    lp->left = left;
}
/**
 * Set the right pointer
 * @param lp the linkpair to set it for
 * @param right the next pair on the right
 */
void linkpair_set_right( linkpair *lp, linkpair *right )
{
    lp->right = right;
}
/**
 * Get the left pointer
 * @param lp the linkpair in question
 * @return the next pair on the left
 */
linkpair *linkpair_left( linkpair *lp )
{
    return lp->left;
}
/**
 * Get the right pointer
 * @param lp the linkpair in question
 * @return the next pair on the right
 */
linkpair *linkpair_right( linkpair *lp )
{
    return lp->right;
}
/**
 * Set the index into the suffixtree text
 * @param lp the linkpair to set it for
 * @param st_off the index
 */
void linkpair_set_st_off( linkpair *lp, int st_off )
{
    lp->st_off = st_off;
}
/**
 * Get the index into the suffixtree text
 * @param lp the linkpair to set it for
 * @return the index
 */
int linkpair_st_off( linkpair *lp )
{
    return lp->st_off;
}
/**
 * Get the pair associated with this linkpair
 * @param lp the linkpair
 * @return its pair
 */
pair *linkpair_pair( linkpair *lp )
{
    return lp->p;
}
/**
 * Is a linkpair the trailing arc of a node (or a hint)?
 * @param lp the linkpair to test
 * @return 1 if it is, else 0
 */
int linkpair_trailing_node( linkpair *lp )
{
    if ( lp->left != NULL )
    {
        if ( pair_is_hint(lp->left->p)
            || bitset_intersects(pair_versions(lp->left->p),
            pair_versions(lp->p)) )
            return 1;
    }
    return 0;
}
/**
 * Get the next pair in an array
 * @param pairs a list of linkpairs
 * @param lp the first linkpair to look at
 * @param bs the bitset of versions to follow
 * @return NULL on failure else the the next relevant linkpair
 */
linkpair *linkpair_next( linkpair *lp, bitset *bs )
{
    lp = lp->right;
    while ( lp != NULL 
        && !bitset_intersects(bs,pair_versions(linkpair_pair(lp))) )
        lp = linkpair_right(lp);
    return lp;
}
/**
 * is this linkpair free? i.e. is it not the trailing pair of a node?
 * @param lp the linkpair to test
 * @return 1 if it is free else 0
 */
int linkpair_free( linkpair *lp )
{
    if ( lp->left == NULL )
        return 1;
    else
    {
        pair *p = linkpair_pair(lp->left);
        if ( pair_is_hint(p) )
            return 0;
        else
        {
            pair *q = linkpair_pair(lp);
            return bitset_intersects(pair_versions(p),pair_versions(q));
        }
    }
}
/**
 * Add a hint to the node immediately before the given pair
 * @param lp the linkpair trailing pair of the node
 * @param version the version of the hint
 * @param log the lg to record errors in
 */
void linkpair_add_hint( linkpair *lp, int version, plugin_log *log )
{
    pair *p = lp->left->p;
    if ( pair_is_hint(p) )
        bitset_set(pair_versions(p),version);
    else if ( !bitset_next_set_bit(pair_versions(p),version==version) )
    {
        bitset *bs = bitset_create();
        bitset_set( bs, version );
        pair *hint = pair_create_hint( bs );
        linkpair *hint_lp = linkpair_create( hint, log );
        hint_lp->left = lp->left;
        hint_lp->right = lp;
        lp->left->right = hint_lp;
        lp->left = hint_lp;
    }
}
/**
 * Split a linkpair and adjust parent/child if not a basic pair
 * @param lp the linkpair to split
 * @param at the offset in the pair data before which to split
 * @param log the log to record errors in
 */
void linkpair_split( linkpair *lp, int at, plugin_log *log )
{
    if ( pair_is_ordinary(lp->p) )
    {
        pair *q = pair_split( &lp->p, at );
        if ( q != NULL )
        {
            linkpair *lp2 = linkpair_create( q, log );
            if ( lp2 != NULL )
            {
                lp2->right = lp->right;
                if ( lp2->right != NULL )
                    lp2->right->left = lp2;
                lp->right = lp2;
                lp2->left = lp;
            }
        }
    }
    else if ( pair_is_child(lp->p) )
    {
        // to do
    }
    else if ( pair_is_parent(lp->p) )
    {
        // to do
    }
}
/**
 * Convert a list of linkpairs to a standard pair array
 * @param lp the head of the linkpair list
 * @return an allocated dynamic array of pairs or NULL
 */
dyn_array *linkpair_to_pairs( linkpair *lp )
{
    int npairs=0;
    linkpair *temp = lp;
    while ( temp != NULL )
    {
        npairs++;
        temp = temp->right;
    }
    dyn_array *da = dyn_array_create( npairs );
    if ( da != NULL )
    {
        temp = lp;
        while ( temp != NULL )
        {
            dyn_array_add( da, temp->p );
            temp = temp->right;
        }
    }
    return da;
}
/**
 * Do we define a node immediately on our right?
 * @param lp the linkpair in question
 * @return 1 if we do else 0
 */
int linkpair_node_to_right( linkpair *lp )
{
    int res = 0;
    if ( lp->right != NULL )
    {
        if ( pair_is_hint(lp->right->p) )
            res = 1;
        else
        {
            bitset *bs1 = pair_versions(lp->p);
            bitset *bs2 = pair_versions(lp->right->p);
            res = bitset_intersects(bs1,bs2);
        }
    }
    return res;
}
/**
 * Do we define a node immediately on our left?
 * @param lp the linkpair in question
 * @return 1 if we do else 0
 */
int linkpair_node_to_left( linkpair *lp )
{
    int res = 0;
    if ( lp->left != NULL )
    {
        if ( pair_is_hint(lp->left->p) )
            res = 1;
        else
        {
            bitset *bs1 = pair_versions(lp->p);
            bitset *bs2 = pair_versions(lp->left->p);
            res = bitset_intersects(bs1,bs2);
        }
    }
    return res;
}
/**
 * Compute the overhang for a linkpair node
 * @param lp the leading pair of a node
 * @return the bitset of the overhang or NULL (user to free)
 */
bitset *linkpair_node_overhang( linkpair *lp )
{
    bitset *bs = bitset_create();
    bs = bitset_or( bs, pair_versions(lp->p) );
    if ( pair_is_hint(lp->right->p) )
    {
        bs = bitset_or( bs, pair_versions(lp->right->p) );
        lp = lp->right;
    }
    bitset_and_not( bs, pair_versions(lp->right->p) );
    return bs;
}
/**
 * Add a linkpair after a node. lp->right will be displaced by one 
 * linkpair. Although this will increasse the node's overhang, the increase
 * is exactly that of the displaced linkpair's versions, which will reattach 
 * by the overhang, rather than the forced, rule.
 * @param lp the incoming pair of a node
 * @param after the linkpair to add into lp's node. must intersect with lp.
 * @return 1 if the new node is a bona fide node
 */
int linkpair_add_at_node( linkpair *lp, linkpair *after )
{
    bitset *bs1 = pair_versions(lp->p);
    if ( pair_is_hint(linkpair_pair(lp->right)) ) 
        lp = lp->right;
    after->right = lp->right;
    after->left = lp;
    if ( lp->right != NULL )
        lp->left = after;
    lp->right = after;
    return bitset_intersects( bs1, pair_versions(after->p) );
}
/**
 * Add a linkpair after a given point, creating a new node.
 * @param lp the point to add it after
 * @param after the linkpair to become after lp
 * @return 1 if successful else 0
 */
int linkpair_add_after( linkpair *lp, linkpair *after )
{
    if ( !linkpair_node_to_right(lp) )
    {
        after->right = lp->right;
        lp->right = after;
        after->left = lp;
        return 1;
    }
    else
        return 0;
}
#ifdef LINKPAIR_TEST
int main( int argc, char **argv )
{
    printf("size of linkpair: %zu\n",sizeof(linkpair));
}
#endif