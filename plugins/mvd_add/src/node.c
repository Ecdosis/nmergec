/*
 * Copyright 2013 Desmond Schmidt
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "plugin_log.h"
#include "node.h"
/**
 * Represent the node structure of a tree but don't build it here
 */
struct node_struct
{
    // index into string: edge leading INTO node
    int start;
    // length of match or INFINITY
    int len;
    // pointer to next sibling
    node *next;
    // pointer to first child
    node *children;
    // suffix link 
    node *link;
    // parent of node : needed to implement splits
    node *parent;
};
/**
 * Create a node safely
 * @return the finished node or fail
 */
node *node_create( int start, int len, plugin_log *log )
{
    node *n = calloc( 1, sizeof(node) );
    if ( n == NULL )
        plugin_log_add(log, "couldn't create new node\n" );
    else
    {
        n->start = start;
        n->len = len;
    }
    return n;
}
/**
 * Create a leaf starting at a given offset 
 * @param i the offset into the string
 * @return the finished leaf or NULL on failure
 */
node *node_create_leaf( int i, plugin_log *log )
{
    node *leaf = calloc( 1, sizeof(node) );
    if ( leaf != NULL )
    {
        leaf->start = i;
        leaf->len = INFINITY;
    }
    else
        plugin_log_add(log,"tree: failed to create leaf\n");
    return leaf;
}
/**
 * Dispose of a node recursively, and thus the entire tree
 * @param v the node to dispose and its children. 
 */
void node_dispose( node *v )
{
    node *child = v->children;
    while ( child != NULL )
    {
        node *next = child->next;
        node_dispose( child );
        child = next;
    }
    free( v );
}
/**
 * Add an initially single-char leaf to the tree
 * @param parent the node to hang it off
 * @param i start-index in str of the leaf
 * @param log the log to record error messages in
 */
int node_add_leaf( node *parent, int i, plugin_log *log )
{
    int res = 0;
    node *leaf = node_create_leaf( i, log );
    if ( leaf != NULL )
    {
        node_add_child( parent, leaf );
        res = 1;
    }
    return res;
}
/**
 * Add a child node (can't fail)
 * @param parent the node to add the child to
 * @param child the child to add
 */
void node_add_child( node *parent, node *child )
{
    if ( parent->children == NULL )
        parent->children = child;
    else
    {
        node *temp = parent->children;
        while ( temp->next != NULL )
            temp = temp->next;
        temp->next = child;
    }
    child->parent = parent;
}
/**
 * Is this node the last one in this branch of the tree?
 * @param v the node to test
 * @return 1 if it is else 0
 */
int node_is_leaf( node *v )
{
    return v->children == NULL;
}
/**
 * Split this node's edge by creating a new node in the middle. Remember 
 * to preserve the "once a leaf always a leaf" property or f will be wrong.
 * @param v the node in question
 * @param loc the place on the edge after which to split it
 * @param log the log to record errors in
 * @return the new internal node
 */
node *node_split( node *v, int loc, plugin_log *log )
{
    // create front edge leading to internal node u
    int u_len = loc-v->start+1;
    node *u = node_create( v->start, u_len, log );
    // now shorten the following node v
    if ( !node_is_leaf(v) )
        v->len -= u_len;
    v->start = loc+1;
    // replace v with u in the children of v->parent
    node *child = v->parent->children;
    node *prev = child;
    while ( child != NULL && child != v )
    {
        prev = child;
        child = child->next;
    }
    if ( child == prev )
        v->parent->children = u;
    else
        prev->next = u;
    u->next = child->next;
    v->next = NULL;
    // reset parents
    u->parent = v->parent;
    v->parent = u;
    u->children = v;
    return u;
}
/**
 * Set the node's suffix link
 * @param v the node in question
 * @param link the node sv
 */
void node_set_link( node *v, node *link )
{
    v->link = link;
}
void node_set_len( node *v, int len )
{
    v->len = len;
}
node *node_parent( node *v )
{
    return v->parent;
}
/**
 * Get the suffix link
 * @param v the node to get the link of
 * @return the node sv
 */
node *node_link( node *v )
{
    return v->link;
}
//accessors
int node_len( node *v )
{
    return v->len;
}
int node_start( node *v )
{
    return v->start;
}
node *node_next( node *v )
{
    return v->next;
}
node *node_children( node *v )
{
    return v->children;
}
int node_end( node *v, int max )
{
    if ( node_len(v) == INFINITY )
        return max;
    else
        return v->start+v->len-1;
}
