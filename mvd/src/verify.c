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

static void check_dangling( dyn_array *dangling, dyn_array *nodes, vgnode *n,
    pair *exclude, int index )
{
    int k=0;
    while ( k<dyn_array_size(dangling) )
    {
        pair *q = (pair*)dyn_array_get(dangling,k);
        if ( q != exclude )
        {
            if ( vgnode_wants_incoming(n,q) )
            {
                // rule 3: incoming
                dyn_array_remove( dangling, k );
                k--;
                vgnode_add_incoming( n, q );
                if ( vgnode_balanced(n) )
                {
                    dyn_array_remove(nodes, index);
                    vgnode_dispose( n );
                    break;
                }
            }
        }
        k++;
    }
}
int find_index( dyn_array *dangling, pair *p )
{
    int i;
    for ( i=0;i<dyn_array_size(dangling);i++ )
    {
        pair *q = (pair*)dyn_array_get(dangling,i);
        if ( q == p )
            return i;
    }
    return -1;
}
int verify_check( dyn_array *pairs )
{
    int res = 1;
    int i;
    dyn_array *dangling = dyn_array_create( 10 );
    dyn_array *nodes = dyn_array_create( 10 );
    bitset *all = bitset_create();
    pair *prev = NULL;
    for ( i=0;i<dyn_array_size(pairs);i++ )
    {
        pair *p = (pair*)dyn_array_get(pairs,i);
        bitset *pv = pair_versions(p);
        bitset_or( all, pv );
        if (i==19)
            printf("19\n");
        if ( prev != NULL
            && bitset_intersects(pv,pair_versions(prev)) )
        {
            vgnode *n = vgnode_create();
            if ( n != NULL )
            {
                // 1. forced rule
                vgnode_add_incoming( n, prev );
                vgnode_add_outgoing( n, p );
                dyn_array_add( dangling, p );
                int index = find_index(dangling,prev);
                if ( index < 0 )
                {
                    printf("dangling arc not found\n");
                    res = 0;
                }
                else
                    dyn_array_remove( dangling, index );
                if ( vgnode_balanced(n) )
                {
                    vgnode_dispose( n );
                    n = NULL;
                }
                else
                {
                    dyn_array_add( nodes, n );
                    check_dangling( dangling, nodes, n, p, dyn_array_size(nodes)-1 );
                }
            }
        }
        else
        {
            // try to attach it as outgoing from some existing node
            int j;
            int placed = 0;
            for ( j=dyn_array_size(nodes)-1;j>=0;j-- )
            {
                vgnode *vg = (vgnode*)dyn_array_get(nodes,j);
                if ( vgnode_wants(vg,p) )
                {
                    // rule 2: overhang 
                    vgnode_add_outgoing( vg, p );
                    placed = 1;
                    dyn_array_add( dangling, p );
                    if ( vgnode_balanced(vg) )
                    {
                        dyn_array_remove(nodes,j);
                        vgnode_dispose(vg);
                    }
                    else
                        check_dangling( dangling, nodes, vg, p, 
                            dyn_array_size(nodes)-1 );
                    break;
                }
            }
            if ( !placed )
            {
                if ( dyn_array_size(nodes)>0 )
                {
                    printf("Verify: failed to place arc\n");
                    res = 0;
                }
                else
                    dyn_array_add( dangling, p );
            }
        }
        prev = p;
        if ( bitset_equals(pv,all) && dyn_array_size(dangling)!=1 )
            printf("too many items in dangling!\n");
    }
    bitset *check = bitset_create();
    for ( i=0;i<dyn_array_size(dangling);i++ )
    {
        pair *p = (pair*)dyn_array_get( dangling, i );
        bitset_or( check, pair_versions(p) );
    }
    if ( !bitset_equals(check, all) )
    {
        printf("verify: closing arcs don't match all versions\n");
        res = 0;
    }
    if ( res )
        res = dyn_array_size(nodes)==0;
    dyn_array_dispose(nodes );
    dyn_array_dispose( dangling );
    bitset_dispose( all );
    bitset_dispose( check );
    return res;
}