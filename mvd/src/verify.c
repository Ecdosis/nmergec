#include <stdlib.h>
#include <stdio.h>
#include "unicode/uchar.h"
#include "version.h"
#include "dyn_array.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "mvd.h"
#include "verify.h"
#include "vgnode.h"
#include "hint.h"
#include "mvd.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/**
 * Verify that an MVD conforms to its own internal rules
 */
struct verify_struct
{
    dyn_array *nodes;
    dyn_array *dangling;
    bitset *all;
    int npairs;
    // not owned by us
    dyn_array *pairs;
};
/**
 * Create a verify object
 * @param pairs the mvd pairs array
 * @param nversions the number of versions in the MVD
 * @return the completed object or NULL on failure
 */
verify *verify_create( dyn_array *pairs, int nversions )
{
    verify *v = calloc( 1, sizeof(verify) );
    if ( v != NULL )
    {
        int i;
        v->pairs = pairs;
        v->npairs = dyn_array_size( v->pairs );
        v->nodes = dyn_array_create( v->npairs/2 );
        v->all = bitset_create();
        for ( i=1;i<=nversions;i++ )
            bitset_set( v->all, i );
        v->dangling = dyn_array_create( 12 );
        if ( v->nodes==NULL||v->dangling==NULL||v->all==NULL )
        {
            fprintf(stderr,"mvd-verify: failed to initialise object\n");
            verify_dispose(v);
            v = NULL;
        }
    }
    else
        fprintf(stderr,"mvd-verify: failed to create verify object\n");
    return v;
}
/**
 * Dispose of the verifier
 * @param v the verifier to dispose
 */
void verify_dispose( verify *v )
{
    if ( v->all != NULL )
        bitset_dispose( v->all );
    if ( v->dangling != NULL )
        dyn_array_dispose( v->dangling );
    if ( v->nodes != NULL )
    {
        int i,nsize=dyn_array_size(v->nodes);
        for ( i=0;i<nsize;i++ )
        {
            vgnode *n = dyn_array_get(v->nodes,i);
            vgnode_dispose( n );
        }
        dyn_array_dispose( v->nodes );
    }
}
/**
 * Subtract pairs in the dangling set
 * @param v the verification object
 * @param u the node to attach them to
 * @param bs versions of the current pair
 */
static void subtract_dangling( verify *v, vgnode *u, bitset *bs )
{
    int j = dyn_array_size(v->dangling)-1;
    while ( j>=0 )
    {
        pair *p = dyn_array_get( v->dangling, j );
        if ( bitset_intersects(pair_versions(p),bs) )
        {
            if ( !vgnode_check_incoming(u,pair_versions(p)) )
                fprintf(stderr,"extra incoming arc ignored\n");
            else
            {
                dyn_array_remove( v->dangling, j );
                vgnode_add_incoming( u, p );
            }
        }
        j--;
    }
}
/**
 * Verify that the MVD represented here is OK. Print the error if any.
 * @param v the verifier to check
 * @return 1 if it checks out else 0
 */
int verify_check( verify *v )
{
    int i,j;
    int res = 1;
    int noverhangs=0,ndefaults=0,nforced=0;
    pair *prev = NULL;
    pair *current = NULL;
    vgnode *start = vgnode_create(VGNODE_START);
    hint *head = hint_create( v->all, start );
    vgnode *end = vgnode_create(VGNODE_END);
    dyn_array_add( v->nodes, start );
    for ( i=0;i<v->npairs;i++ )
    {
        current = dyn_array_get( v->pairs, i );
        if ( pair_is_hint(current) )
            continue;
        else if ( i>0&& (bitset_intersects(pair_versions(prev),
            pair_versions(current))||pair_is_hint(prev)) )
        {
            vgnode *n = vgnode_create( VGNODE_BODY );
            if ( n != NULL )
            {
                bitset *oh;
                vgnode_add_outgoing( n, current );
                nforced++;
                subtract_dangling( v, n, pair_versions(current) );
                // check for overhang, create pseudo "hint"
                oh = vgnode_overhang( n );
                if ( pair_is_hint(prev) )
                {
                    oh = bitset_or( oh, pair_versions(prev) );
                    bitset_clear_bit( oh, 0 );
                }
                if ( oh != NULL )
                {
                    if ( !bitset_empty(oh) )
                    {
                        hint *h = hint_create( oh, n );
                        if ( head == NULL )
                            head = h;
                        else
                            hint_append( head, h );
                    }
                }
                bitset_dispose( oh );
                dyn_array_add( v->nodes, n );
                dyn_array_add( v->dangling, current );
            }
            else
                break;
        }
        else
        {
            hint *h = hint_contains(head,pair_versions(current));
            // look for a node to attach it to
            if ( h != NULL )
            {
                vgnode *hn = hint_node( h );
                if ( hint_subtract(h,pair_versions(current)) )
                {
                    head = hint_delist(h);
                    hint_dispose( h );
                }
                vgnode_add_outgoing( hn, current );
                noverhangs++;
                subtract_dangling( v, hn, pair_versions(current) );
            }
            else
            {
                // attach to last node by default
                ndefaults++;
                int end_index = dyn_array_size(v->nodes)-1;
                vgnode *u = dyn_array_get(v->nodes,end_index);
                vgnode_add_outgoing( u, current );
                subtract_dangling( v, u, pair_versions(current) );
            }
            dyn_array_add( v->dangling, current );
        }
        prev = current;
    }
    dyn_array_add( v->nodes, end );
    for ( j=0;j<dyn_array_size(v->dangling);j++ )
    {
        pair *p = dyn_array_get(v->dangling,j);
        vgnode_add_incoming( end, p );
    }
    printf("nforced=%d noverhangs=%d ndefaults=%d\n",nforced,noverhangs,ndefaults);
    // verify that all nodes are balanced
    int nsize = dyn_array_size(v->nodes);
    for ( i=0;i<nsize;i++ )
    {
        vgnode *n = dyn_array_get( v->nodes, i );
        if ( !vgnode_verify(n,v->all) )
        {
            char str1[32],str2[32];
            vgnode_inversions(n,str1,32);
            vgnode_outversions(n,str2,32);
            fprintf(stderr,"verify: vgnode %d of %d unbalanced: "
                "incoming: %s, outgoing: %s\n",i,nsize,str1,str2);
            res = 0;
            break;
        }
    }
    return res;
}