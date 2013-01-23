#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#include "mvd/pair.h"
#include "link_node.h"
// assuming we are on an 8 byte system
#define DATA_MINSIZE 5
#define BASIC_PAIR 0
#define CHILD_PAIR 1
#define PARENT_PAIR 2
// this should usually be at least of size 24 
struct pair_struct
{
    bitset *versions;
    union
    {
        link_node *children;
        pair *parent;
    };
    short len;
	unsigned char type;
    unsigned char data[DATA_MINSIZE];
};
/**
 * Create a basic pair. 
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length
 * @return an allocate pair object or NULL
 */
pair *pair_create_basic( bitset *versions, unsigned char *data, int len )
{
    size_t extraDataSize = (len-DATA_MINSIZE>0)?len-DATA_MINSIZE:0;
    pair *p = calloc( 1, sizeof(pair)+extraDataSize );
    if ( p != NULL )
    {
        p->versions = bitset_clone(versions);
        memcpy( p->data, data, len );
        p->type = BASIC_PAIR;
        p->len = len;
    }
    else
        fprintf( stderr,"pair: failed to create basic pair\n");
    return p;
}
/**
 * Create a child pair (set parent later). 
 * @param versions the versions of this bitset
 * @return an allocate pair object or NULL
 */
pair *pair_create_child( bitset *versions )
{
    pair *p = calloc( 1, sizeof(pair) );
    if ( p != NULL )
    {
        p->versions = versions;
        p->type = CHILD_PAIR;
    }
    else
        fprintf( stderr,"pair: failed to create child pair\n");
    return p;
}
/**
 * Create a child pair (transpose pair depends on parent). 
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length
 * @return an allocate pair object or NULL
 */
pair *pair_create_parent( bitset *versions, unsigned char *data, int len )
{
    size_t extraDataSize = (len-DATA_MINSIZE>0)?len-DATA_MINSIZE:0;
    pair *p = calloc( 1, sizeof(pair)+extraDataSize );
    if ( p != NULL )
    {
        p->versions = versions;
        memcpy( p->data, data, len );
        p->type = PARENT_PAIR;
        p->len = len;
    }
    else
        fprintf( stderr,"pair: failed to create parent pair\n");
    return p;
}
/**
 * Dispose of a single pair
 * @param p the pair in question
 */
void pair_dispose( pair *p )
{
    bitset_dispose( p->versions );
    free( p );
}
/**
 * Set the parent of this transposed pair
 * @param p the pair to set
 * @param parent its parent
 * @return pointer to the possibly reallocated pair
 */
pair *pair_set_parent( pair *p, pair *parent )
{
    if ( p->type != CHILD_PAIR )
    {
        pair *new = pair_create_child(p->versions);
        // if there was data we lose it here
        pair_dispose( p );
        p = new;
    }
    if ( p != NULL )
        p->parent = parent;
    return p;
}
/**
 * Add a transpose child to this parent
 * @param p the parent to adopt the child
 * @param child the child to adopt
 * @return pointer to the new (or old) pair or NULL if it failed
 */
pair *pair_add_child( pair *p, pair *child )
{
    // get the type right first
    if ( p->type != PARENT_PAIR )
    {
        pair *new = pair_create_parent( p->versions, p->data, p->len );
        pair_dispose( p );
        p = new;
    }
    if ( p != NULL )
    {
        link_node *ln = link_node_create( child );
        if ( ln != NULL )
        {
            if ( p->children == NULL )
                p->children = ln;
            else
                link_node_append( p->children, ln );
        }
        else
        {
            fprintf(stderr,"pair: failed to add child\n");
            pair_dispose( p );
            p = NULL;
        }
    }
    return p;
}
/**
 * Is this pair a child of something?
 * @param p the pair in question
 * @return 1 if it is else 0
 */
int pair_is_child( pair *p )
{
    return p->parent != NULL;
}
#ifdef DEBUG_PAIR
#include <stdio.h>
int main( int argc, char **argv )
{
    struct basic_pair
    {
        unsigned char *data;
        bitset *versions;
        int len;
    };
    struct child_pair
    {
        pair *parent;
        bitset *versions;
    };
    struct parent_pair
    {
        unsigned char *data;
        int len;
        bitset *versions;
        link_node *children;
    };
    printf("sizeof(struct basic_pair)=%zu sizeof(struct child_pair)=%zu "
        "sizeof(struct parent_pair)=%zu\n",
        sizeof(struct basic_pair),sizeof(struct child_pair),
        sizeof(struct parent_pair));
}
#endif