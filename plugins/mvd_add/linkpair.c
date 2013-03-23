#include <stdio.h>
#include "unicode/uchar.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "linkpair.h"
#include "plugin_log.h"
#include "dyn_array.h"
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
        plugin_log_add( "linkpair: failed to create object\n");
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
void linkpair_left( linkpair *lp )
{
    return lp->left;
}
/**
 * Get the right pointer
 * @param lp the linkpair in question
 * @return the next pair on the right
 */
void linkpair_right( linkpair *lp )
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
void linkpair_st_off( linkpair *lp )
{
    return lp->st_off;
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
 * Fix a newly split-off trailing or inserted linkpair 
 * @param lp the linkpair to fix
 */
static void linkpair_fix( linkpair *lp )
{
    // 1. find all pairs *implicitly* attached to lp->left+lp
    // that were orphaned when this pair was split
    linkpair *temp = lp->right;
    dyn_array *implicits = dyn_array_create( IMPLICIT_SIZE );
    if ( implicits != NULL )
    {
        bitset *bs = bitset_create();
        while ( temp != NULL )
        {
            if ( linkpair_trailing_node(temp) )
                break;
            else
            {
                bitset_or( bs, pair_versions(temp->p) );
                dyn_array_add( implicits, temp->p );
            }
            temp = temp->right;
        }
        // 2. prune implicits
        temp = lp->left;
        while ( temp != NULL && !bitset_empty(bs) )
        {
            if ( pair_is_hint(temp->p) )
            {
                // remove intersecting pairs from the implicits
                int i;
                for ( i=0;i<dyn_array_size(implicits);i++ )
                {
                    pair *q = (pair*)dyn_array_get(implicits,i);
                    bitset *qv = pair_versions(q);
                    bitset *hv = pair_versions(temp->p);
                    if ( bitset_intersects(qv,hv) )
                    {
                        dyn_array_remove(implicits,i);
                        bitset_and_not( bs, qv );
                    }
                }
            }
            else
            {
                bitset_and_not( bs, pair_versions(temp->p) );
            }
            if ( linkpair_trailing_node(temp) )
                break;
            temp = temp->left;
        }
        // 3. ensure that all implicits attach to the node preceding lp
        temp = lp->left;
        // 3a. get the versions of the preceding node
        bitset *nbs = bitset_create();
        linkpair hint = NULL;
        linkpair *node = NULL;
        while ( temp->left != NULL )
        {
            bitset *pv = pair_versions(temp->p);
            bitset *lpv = pair_versions(temp->left->p);
            if ( pair_is_hint(temp->left->p) )
            {
                bitset_or( nbs, pair_versions(temp->left->left->p) );
                bitset_or( nbs, pv );
                bitset_or( nbs, lpv );
                hint = temp->left->p;
                break;
            }
            else if ( bitset_intersects(pv,lpv) )
            {
                bitset_or( nbs, pv );
                bitset_or( nbs, lpv );
                node = temp;
                break;
            }
            temp = temp->left;
        }
        // 4a. subtract the node versions from the implicit versions 
        bitset_and_not( bs, nbs );
        // 4b. Add the remaining versions to the hint
        if ( !bitset_empty(bs) )
        {
            if ( hint != NULL )
                bitset_or( bs,pair_versions(hint) );
            else
            {
                bitset_set( bs, 0 );
                pair *r = pair_create_basic( bs, NULL, 0 );
                if ( r != NULL )
                {
                    linkpair h = linkpair_create( r );
                    if ( h != NULL )
                    {
                        h->left = node->left;
                        node->left = h;
                    }
                }
            }
        }
        dyn_array_dispose( implicits );
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
            linkpair *lp2 = linkpair_create( q );
            if ( lp2 != NULL )
            {
                lp2->right = lp->right;
                lp->right = lp2;
            }
            linkpair_fix( lp2 );
        }
    }
    else if ( pair_is_child(lp->p) )
    {
        
    }
    else if ( pair_is_parent(lp->p) )
    {
        
    }
}
#ifdef LINKPAIR_TEST
int main( int argc, char **argv )
{
    printf("size of linkpair: %zu\n",sizeof(linkpair));
}
#endif