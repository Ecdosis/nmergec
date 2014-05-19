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
    vgnode_reset();
    for ( i=0;i<dyn_array_size(pairs);i++ )
    {
        pair *p = (pair*)dyn_array_get(pairs,i);
        bitset *pv = pair_versions(p);
        bitset_or( all, pv );
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
                    char *str = pair_tostring(p);
                    if ( str != NULL )
                    {
                        printf("dangling arc %s not found\n",str);
                        free( str );
                    }
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
                    int k;
                    for ( k=i-1;k>=0;k-- )
                    {
                        pair *q = dyn_array_get(pairs,k);
                        if ( bitset_intersects(pair_versions(q),pair_versions(p)) )
                            break;
                    }
                    if ( k != -1 )
                    {
                        char *str = pair_tostring(p);
                        if ( str != NULL )
                        {
                            printf("Verify: failed to place arc %s\n",str);
                            free( str );
                        }
                        res = 0;
                    }
                    else
                        placed = 1;
                }
                else
                    placed = 1;
                if ( placed )
                    dyn_array_add( dangling, p );
            }
        }
        prev = p;
        if ( bitset_equals(pv,all) && dyn_array_size(dangling)!=1 )
            printf("too many items (%d) in dangling!\n",dyn_array_size(dangling));
    }
    bitset *check = bitset_create();
    for ( i=0;i<dyn_array_size(dangling);i++ )
    {
        pair *p = (pair*)dyn_array_get( dangling, i );
        bitset_or( check, pair_versions(p) );
    }
    if ( !bitset_equals(check, all) )
    {
        //bitset *bs, char *dst, int len
        char cvers[12],avers[12];
        bitset_tostring(check,cvers,12);
        bitset_tostring(all,avers,12);
        printf("verify: closing arcs (%s) don't match all versions (%s)\n",
            cvers,avers);
        res = 0;
    }
    if ( res )
    {
        if ( dyn_array_size(nodes)!= 0 )
        {
            printf("%d nodes left over at end of graph verification\n",
                dyn_array_size(nodes));
            res = dyn_array_size(nodes)==0;
            int k;
            for ( k=0;k<dyn_array_size(nodes);k++ )
            {
                vgnode *vg = (vgnode*)dyn_array_get(nodes,k);
                if ( !vgnode_balanced(vg) )
                {
                    char *str = vgnode_tostring(vg);
                    if ( str != NULL )
                    {
                        printf("node %s unbalanced\n",str);
                        free( str );
                    }
                }
                vgnode_dispose( vg );
            }
        }
    }
    dyn_array_dispose(nodes );
    dyn_array_dispose( dangling );
    bitset_dispose( all );
    bitset_dispose( check );
    return res;
}