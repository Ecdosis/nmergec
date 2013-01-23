/* 
 * File:   linked_list.h
 * Author: desmond
 *
 * Created on January 19, 2013, 7:41 AM
 */

#ifndef LINK_NODE_H
#define	LINK_NODE_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct link_node_struct link_node;
link_node *link_node_create();
void link_node_set_obj( link_node *ln, void *obj );
void link_node_append( link_node *ln, link_node *next );
link_node *link_node_next( link_node *ln );
void *link_node_obj( link_node *ln );
void link_node_dispose( link_node *ln );
#ifdef MVD_TEST
int test_link_node( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* LINK_NODE_H */

