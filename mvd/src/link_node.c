#include <stdlib.h>
#include <stdio.h>
#include "link_node.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/**
 * General purpose list node object
 */
struct link_node_struct
{
    void *obj;
    link_node *next;
};
/**
 * Create a list node for an object
 * @param obj the anonymous object
 * @return the node that wraps it or NULL
 */
link_node *link_node_create()
{
    link_node *ln = calloc( 1, sizeof(link_node) );
    if ( ln == NULL )
        fprintf(stderr,"link_node: failed to allocate node\n");
    return ln;
}
/**
 * Dispose of a link node and all its children (but not contents)
 * @param ln the link node in question
 */
void link_node_dispose( link_node *ln )
{
    while ( ln != NULL )
    {
        link_node *next = ln->next;
        free( ln );
        ln = next;
    }
}
/**
 * Set the object in an existing link_node
 * @param ln the link node in question
 * @param obj the object it is to contain
 */
void link_node_set_obj( link_node *ln, void *obj )
{
    ln->obj = obj;
}
/**
 * Create a list node for an object
 * @param ln the link node to append the new object to
 * @param next the new next node
 * @return the node that wraps it or NULL
 */
void link_node_append( link_node *ln, link_node *next )
{
    link_node *temp = ln;
    while ( temp->next != NULL )
        temp = temp->next;
    temp->next = next;
}
/**
 * Get the next node in the list
 * @param ln the current link_node
 * @return the next one if any
 */
link_node *link_node_next( link_node *ln )
{
    return ln->next;
}
/**
 * Get the object stored in this node
 * @param ln the current link_node
 * @return the node's content
 */
void *link_node_obj( link_node *ln )
{
    return ln->obj;
}
#ifdef MVD_TEST
#include <string.h>
#include <stdio.h>
void test_link_node( int *passed, int *failed )
{
    link_node *ln1 = link_node_create();
    link_node_set_obj( ln1, "banana" );
    link_node *ln2 = link_node_create();
    link_node_set_obj( ln2, "apple" );
    link_node *ln3 = link_node_create();
    link_node_set_obj( ln3, "pineapple" );
    link_node_append( ln1, ln2 );
    link_node_append( ln1, ln3 );
    link_node *temp = ln1;
    int failure = 0;
    if ( strcmp(link_node_obj(temp),"banana")!=0 )
    {
        failure++;
        fprintf(stderr,"link_node: link contents invalid\n");
    }
    temp = link_node_next( temp );
    if ( strcmp(link_node_obj(temp),"apple")!=0 )
    {
        failure++;
        fprintf(stderr,"link_node: link contents invalid\n");
    }
    temp = link_node_next( temp );
    if ( strcmp(link_node_obj(temp),"pineapple")!=0 )
    {
        failure++;
        fprintf(stderr,"link_node: link contents invalid\n");
    }
    if ( failure > 0 )
        *failed += 1;
    else
        *passed += 1;
    link_node_dispose( ln1 );
}
#endif
