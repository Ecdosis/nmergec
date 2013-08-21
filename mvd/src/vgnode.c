/**
 * Node representation of variant graph node for testing and verification
 */
#include <stdlib.h>
#include <stdio.h>
#include "unicode/uchar.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "vgnode.h"
#include "dyn_array.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
struct vgnode_struct
{
    dyn_array *incoming;
    dyn_array *outgoing;
    int kind;
};
/**
 * Create a vgnode object
 * @param kind the kind of the vgnode: start, end or body
 * @return the created vgnode or NULL
 */
vgnode *vgnode_create( int kind )
{
    vgnode *n = calloc( 1, sizeof(vgnode) );
    if ( n != NULL )
    {
        n->kind = kind;
        n->incoming = dyn_array_create( 3 );
        n->outgoing = dyn_array_create( 3 );
        if ( n->incoming == NULL || n->outgoing==NULL )
        {
            vgnode_dispose( n );
            n = NULL;
        }
    }
    else
        fprintf(stderr,"vgnode: failed to allocate object\n");
    return n;
}
/**
 * Dispose of a vgnode
 * @param n the vgnode to dispose
 */
void vgnode_dispose( vgnode *n )
{
    if ( n->incoming != NULL )
        dyn_array_dispose( n->incoming );
    if ( n->outgoing != NULL )
        dyn_array_dispose( n->outgoing );
}
/**
 * Add a pair to the incoming set
 * @param n the ndoe in question
 * @param p the pair to add
 */
void vgnode_add_incoming( vgnode *n, pair *p )
{
    if ( n->kind != VGNODE_START )
        dyn_array_add( n->incoming, p );
}
/**
 * Add a pair to the outgoing set
 * @param n the vgnode in question
 * @param p the pair to add
 */
void vgnode_add_outgoing( vgnode *n, pair *p )
{
    if ( n->kind != VGNODE_END )
    {
        bitset *bs = bitset_create();
        if ( bs != NULL )
        {
            int i;
            for ( i=0;i<dyn_array_size(n->outgoing);i++ )
            {
                pair *q = dyn_array_get( n->outgoing, i );
                bs = bitset_or( bs, pair_versions(q) );
            }
            if ( bitset_intersects( bs, pair_versions(p)) )
                fprintf(stderr,"vgnode: unwanted outgoing arc\n");  
            bitset_dispose( bs );
        }
        dyn_array_add( n->outgoing, p );
    }
}
/**
 * Verify that this vgnode is balanced (incoming=outgoing versions)
 * @param n the vgnode in question
 * @param all the set of all versions in the MVD
 * @return 1 if it checked out, else 0
 */
int vgnode_verify( vgnode *n, bitset *all )
{
    int res = 1;
    if ( n->kind == VGNODE_START )
    {
        int i;
        bitset *bs1 = bitset_create();
        for ( i=0;i<dyn_array_size(n->outgoing);i++ )
        {
            bitset *pv = pair_versions(dyn_array_get(n->outgoing,i));
            if ( bitset_intersects( pv, bs1) )
                res = 0;
            bs1 = bitset_or( bs1, pv );
        }
        if ( res )
            res = bitset_equals( bs1, all );
        bitset_dispose( bs1 );
    }
    else if ( n->kind == VGNODE_END )
    {
        int i;
        bitset *bs1 = bitset_create();
        for ( i=0;i<dyn_array_size(n->incoming);i++ )
        {
            bitset *pv = pair_versions(dyn_array_get(n->incoming,i));
            if ( bitset_intersects( pv, bs1) )
                res = 0;
            bs1 = bitset_or( bs1, pv );
        }
        if ( res )
            res = bitset_equals( bs1, all );
        bitset_dispose( bs1 );
    }
    else 
    {
        bitset *bs1 = bitset_create();
        bitset *bs2 = bitset_create();
        int i;
        for ( i=0;i<dyn_array_size(n->incoming);i++ )
        {
            bitset *pv = pair_versions(dyn_array_get(n->incoming,i));
            if ( bitset_intersects(pv,bs1) )
                res = 0;
            bs1 = bitset_or( bs1, pv );
        }
        for ( i=0;i<dyn_array_size(n->outgoing);i++ )
        {
            bitset *pv = pair_versions(dyn_array_get(n->outgoing,i));
            if ( bitset_intersects(pv,bs2) )
                res = 0;
            bs2 = bitset_or( bs2, pv );
        }
        if ( res )
            res = bitset_equals( bs1, bs2 );
        bitset_dispose( bs1 );
        bitset_dispose( bs2 );
    }
    return res;
}
/**
 * Print outgoing versions
 * @param v the node to print them from
 * @param dst string to store result
 * @param len its length
 */
void vgnode_outversions( vgnode *n, char *dst, int len )
{
    int i;
    bitset *bs = bitset_create();
    for ( i=0;i<dyn_array_size(n->outgoing);i++ )
        bs = bitset_or( bs, pair_versions(dyn_array_get(n->outgoing,i)) );
    bitset_tostring( bs, dst, len );
    bitset_dispose( bs );
}
/**
 * Print incoming versions
 * @param v the node to print them from
 * @param dst string to store result
 * @param len its length
 */
void vgnode_inversions( vgnode *n, char *dst, int len )
{
    int i;
    bitset *bs = bitset_create();
    for ( i=0;i<dyn_array_size(n->incoming);i++ )
        bs = bitset_or( bs, pair_versions(dyn_array_get(n->incoming,i)) );
    bitset_tostring( bs, dst, len );
    bitset_dispose( bs );
}
/**
 * Check that it is OK to add the pair as incoming
 * @param n the node in question
 * @param pv the versions of the incoming pair to add
 * @return 1 if it was OK
 */
int vgnode_check_incoming( vgnode *n, bitset *pv )
{
    if ( n->kind != VGNODE_START )
    {
        int i;
        bitset *bs = bitset_create();
        for ( i=0;i<dyn_array_size(n->incoming);i++ )
            bs = bitset_or( bs, pair_versions(dyn_array_get(n->incoming,i)) );
        int res = bitset_intersects(bs, pv );
        free( bs );
        return !res;
    }
    else
        return 0;
}
/**
 * Get the bits incoming - outgoing
 * @param n the node in question
 * @return incoming-outgoing bits, user must disppose
 */
bitset *vgnode_overhang( vgnode *n )
{
    int i;
    bitset *bs1 = bitset_create();
    for ( i=0;i<dyn_array_size(n->incoming);i++ )
        bs1 = bitset_or( bs1, pair_versions(dyn_array_get(n->incoming,i)) );
    bitset *bs2 = bitset_create();
    for ( i=0;i<dyn_array_size(n->outgoing);i++ )
        bs2 = bitset_or( bs2, pair_versions(dyn_array_get(n->outgoing,i)) );
    bitset_and_not( bs1, bs2 );
    bitset_dispose( bs2 );
    return bs1;
}
#ifdef MVD_TEST
static UChar ustr1[] = {'a','b','a','c','k',0};
static UChar ustr2[] = {'b','o','g','u','s',0};
static UChar ustr3[] = {'f','i','f','t','h',0};
void test_vgnode( int *passed, int *failed )
{
    char buf[9];
    vgnode *body = vgnode_create( VGNODE_BODY );
    vgnode *start = vgnode_create( VGNODE_START );
    vgnode *end = vgnode_create( VGNODE_END );
    bitset *bs1 = bitset_create();
    bitset *bs2 = bitset_create();
    bitset *bs3 = bitset_create();
    bitset_set(bs1,2);
    bitset_set(bs1,4);
    bitset_set(bs2,1);
    bitset_set(bs2,3);
    bitset_set(bs3,1);
    bitset_set(bs3,2);
    bitset_set(bs3,3);
    bitset_set(bs3,4);
    pair *p1 = pair_create_basic(bs1,ustr1,5);
    pair *p2 = pair_create_basic(bs2,ustr2,5);
    pair *p3 = pair_create_basic(bs3,ustr3,5);
    vgnode_add_incoming(body,p1);
    vgnode_inversions(body,buf,8);
    buf[8] = 0;
    if ( strcmp(buf,"0010100")==0 )
        (*passed)++;
    else
    {
        fprintf(stderr,"vgnode: failed to get inversions\n");
        (*failed)++;
    }
    vgnode_add_incoming(body,p2);
    bitset *bs4 = vgnode_overhang(body);
    if ( bitset_equals(bs4,bs3) )
        (*passed)++;
    else
    {
        fprintf(stderr,"vgnode: failed to get overhang\n");
        (*failed)++;
    }
    vgnode_add_outgoing(body,p3);
    if ( vgnode_verify(body,bs3) )
        (*passed)++;
    else
    {
        fprintf(stderr,"vgnode: failed to verify\n");
        (*failed)++;
    }
    vgnode_add_incoming(start,p1);
    vgnode_inversions(start,buf,8);
    if ( strcmp(buf,"0000000")!=0 )
    {
        fprintf(stderr,"vgnode: added incoming to start node\n");
        (*failed)++;
    }
    else
        (*passed)++;
    vgnode_add_outgoing(end,p3);
    vgnode_outversions(start,buf,8);
    if ( strcmp(buf,"0000000")!=0 )
    {
        fprintf(stderr,"vgnode: added outgoing to end node\n");
        (*failed)++;
    }
    else
        (*passed)++;
    vgnode_dispose( start );
    vgnode_dispose( body );
    vgnode_dispose( end );
    pair_dispose( p1 );
    pair_dispose( p2 );
    pair_dispose( p3 );
}
#endif