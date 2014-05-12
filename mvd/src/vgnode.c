/**
 * Node representation of variant graph node for testing and verification
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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
    bitset *incoming;
    bitset *outgoing;
    int id;
};
static int vgnode_id = 0;
/**
 * Create a vgnode object
 * @return the created vgnode or NULL
 */
vgnode *vgnode_create()
{
    vgnode *n = calloc( 1, sizeof(vgnode) );
    if ( n != NULL )
    {
        n->incoming = bitset_create();
        n->outgoing = bitset_create();
        n->id = ++vgnode_id;
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
        bitset_dispose( n->incoming );
    if ( n->outgoing != NULL )
        bitset_dispose( n->outgoing );
    free( n );
}
/**
 * Add a pair to the incoming set
 * @param n the node in question
 * @param p the pair to add
 * @return 1 if it worked lse 0
 */
int vgnode_add_incoming( vgnode *n, pair *p )
{
    int res = 1;
    bitset *pv = pair_versions(p);
    if ( bitset_intersects(n->incoming, pv) )
    {
        printf("extra incoming arc ignored\n");
        res = 0;
    }
    else
        bitset_or( n->incoming, pv );
    return res;
}
/**
 * Add a pair to the outgoing set
 * @param n the node in question
 * @param p the pair to add
 * @return 1 if it worked else 0
 */
int vgnode_add_outgoing( vgnode *n, pair *p )
{
    int res = 1;
    bitset *pv = pair_versions(p);
    if ( bitset_intersects(n->outgoing, pv) )
    {
        printf("extra outgoing arc ignored\n");
        res = 0;
    }
    else
        bitset_or( n->outgoing, pv );
    return res;
}
int vgnode_balanced( vgnode *n )
{
    return bitset_equals(n->incoming,n->outgoing);
}
int vgnode_wants(vgnode *n, pair *p )
{
    int res = 0;
    bitset *diff = bitset_clone( n->incoming );
    if ( diff != NULL )
    {
        bitset_and_not( diff, n->outgoing );
        if ( bitset_intersects(diff,pair_versions(p)) )
            res = 1;
        bitset_dispose( diff );
    }
    return res;
}
int vgnode_wants_incoming( vgnode *n, pair *p )
{
    int res = 0;
    bitset *diff = bitset_clone( n->outgoing );
    if ( diff != NULL )
    {
        bitset_and_not( diff, n->incoming );
        if ( bitset_intersects(diff, pair_versions(p)) )
            res = 1;
        bitset_dispose( diff );
    }
    return res;
}
char *vgnode_tostring( vgnode *n )
{
    int top_incoming = bitset_top_bit(n->incoming);
    int top_outgoing = bitset_top_bit(n->outgoing);
    int len = top_incoming+top_outgoing+6+log10(n->id);
    char *str = calloc(len,1);
    strcpy(str,"[");
    bitset_tostring(n->incoming,&str[1],top_incoming+2);
    strcat(str,"]");
    int slen = strlen(str);
    snprintf(&str[slen],len-slen,"%d",n->id);
    strcat(&str[strlen(str)],"[");
    bitset_tostring(n->outgoing,&str[strlen(str)],top_outgoing+2);
    strcat(str,"]");
    return str;
}