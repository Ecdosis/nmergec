/*
 *  NMergeC is Copyright 2013 Desmond Schmidt
 * 
 *  This file is part of NMergeC. NMergeC is a C commandline tool and 
 *  static library and a collection of dynamic libraries for merging 
 *  multiple versions into multi-version documents (MVDs), and for 
 *  reading, searching and comparing them.
 *
 *  NMergeC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NMergeC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "plugin_log.h"
#include "unicode/uchar.h"
#include "encoding.h"
#include "node.h"
#include "hashtable.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define MAX_LIST_CHILDREN 6

#if INT_MAX != 2147483647
#error "please adjust masks and macros for int size != 4"
#endif
// adjust these if int size != 4
#define INFINITY INT_MAX
#define LEN_MASK 0x7FFFFFFF
#define KIND_MASK 0x80000000
#define PARENT_HASH(p) (p->len&KIND_MASK)==0x80000000
#define PARENT_LIST(p) (p->len&KIND_MASK)==0
struct node_iterator_struct
{
    int num_nodes;
    int position;
    node **nodes;
};
union child_list
{
    node *children;
    hashtable *ht;
};
/**
 * Represent the node structure of a tree but don't build it here
 */
struct node_struct
{
    // index into string: edge leading INTO node
    int start;
    // length of match or INFINITY, kind in MSB
    unsigned len;
    // next node in sibling list
    node *next;
    // child list/table pointer
    union child_list c;
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
    if ( PARENT_LIST(v) )
    {
        node *child = v->c.children;
        while ( child != NULL )
        {
            node *next = child->next;
            node_dispose( child );
            child = next;
        }
    }
    else if ( PARENT_HASH(v) )
    {
        hashtable_dispose( v->c.ht );
    }
    free( v );
}
/**
 * Add an initially single-char leaf to the tree
 * @param parent the node to hang it off
 * @param i start-index in str of the leaf
 * @param str the text we represent
 * @param log the log to record error messages in
 */
int node_add_leaf( node *parent, int i, UChar *str, plugin_log *log )
{
    int res = 0;
    node *leaf = node_create_leaf( i, log );
    if ( leaf != NULL )
    {
        node_add_child( parent, leaf, str, log );
        res = 1;
    }
    return res;
}
/**
 * Find out the number of children we have
 * @param v the node in question
 * @return an integer
 */
int node_num_children( node *v )
{
    int size = 0;
    if ( PARENT_LIST(v) )
    {
        node *temp = v->c.children;
        while ( temp != NULL )
        {
            size++;
            temp = temp->next;
        }
    }
    else
        size = hashtable_size( v->c.ht );
    return size;
}
/**
 * Iterate through a set of nodes
 * @param parent the parent whose children should be iterated through
 * @return an iterator or NULL if it failed
 */
node_iterator *node_children( node *parent, plugin_log *log )
{
    node_iterator *iter = calloc( 1, sizeof(node_iterator) );
    if ( iter != NULL )
    {
        if ( PARENT_LIST(parent) )
        {
            int size = node_num_children( parent );
            iter->nodes = calloc( size, sizeof(node*) );
            iter->num_nodes = size;
            if ( iter->nodes != NULL )
            {
                int i=0;
                node *v = parent->c.children;
                while ( v != NULL )
                {
                    iter->nodes[i++] = v;
                    v = v->next;
                }
            }
            else
            {
                node_iterator_dispose( iter );
                iter = NULL;
                plugin_log_add(log,
                    "node: failed to allocate iterator nodes\n");
            }
        }
        else
        {
            int size = hashtable_size( parent->c.ht);
            iter->num_nodes = size;
            iter->nodes = calloc( size, sizeof(node*) );
            if ( iter->nodes != NULL )
                hashtable_to_array( parent->c.ht,iter->nodes );
            else
            {
                node_iterator_dispose(iter);
                iter = NULL;
            }
        }
    }
    else
        plugin_log_add( log,"node: failed to allocate iterator\n");
    return iter;
}
/**
 * Throw away the memory occupied by the iterator (nothing else)
 * @param iter the iterator to dispose
 */
void node_iterator_dispose( node_iterator *iter )
{
    if ( iter->nodes != NULL )
        free( iter->nodes );
    free( iter );
}
/**
 * Get the next node pointed to by the iterator
 * @param iter the iterator 
 * @return the next node object
 */
node *node_iterator_next( node_iterator *iter )
{
    if ( iter->position < iter->num_nodes )
        return iter->nodes[iter->position++];
    else
        return NULL;
}
/**
 * Are there any more nodes in this iterator?
 * @param iter the iterator
 * @return 1 if it does else 0
 */
int node_iterator_has_next( node_iterator *iter )
{
    return iter->position < iter->num_nodes;
}
/**
 * Add another child to the sibling list
 * @param parent the parent
 * @param child the new sibling of parent's children
 * @param str the text we represent
 * @param log the log to report errors to
 */
static void node_append_sibling( node *parent, node *child, UChar *str, 
    plugin_log *log )
{
    node *temp = parent->c.children;
    int size = 1;
    while ( temp->next != NULL )
    {
        size++;
        if ( size >= MAX_LIST_CHILDREN )
        {
            hashtable *ht = hashtable_create( parent, str, log );
            if ( ht != NULL )
            {
                parent->c.ht = ht;
                parent->len |= KIND_MASK;
                hashtable_add( parent->c.ht, child, str );
                return;
            }
        }
        temp = temp->next;
    }
    temp->next = child;
}
/**
 * Add a child node (can't fail)
 * @param parent the node to add the child to
 * @param child the child to add
 * @param str the text we represent
 * @param log the log to report errors to
 */
void node_add_child( node *parent, node *child, UChar *str, plugin_log *log )
{
    if ( PARENT_LIST(parent) )
    {
        if ( parent->c.children == NULL )
            parent->c.children = child;
        else
            node_append_sibling( parent, child, str, log );
    }
    else
    {
        int res = hashtable_add( parent->c.ht, child, str );
        if ( !res )
            plugin_log_add(log,"node: failed to add child to hashtable");
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
    if ( PARENT_LIST(v) )
        return v->c.children == NULL;
    else
        return 0;
}
/**
 * Replace one child with another
 * @param v the node to be replaced
 * @param u its replacement
 * @param str the text we represent
 * @param log the log to send errors to
 */
static void replace_child( node *v, node *u, UChar *str, plugin_log *log )
{
    if ( PARENT_LIST(v->parent) )
    {
        // isolate v and repair the list of children
        node *child = v->parent->c.children;
        node *prev = child;
        while ( child != NULL && child != v )
        {
            prev = child;
            child = child->next;
        }
        if ( child == prev )
            v->parent->c.children = u;
        else
            prev->next = u;
        u->next = child->next;
        v->next = NULL;
        //node_print_children(v->parent);
    }
    else if ( PARENT_HASH(v->parent) )
    {
        int res = hashtable_replace( v->parent->c.ht, str, v, u );
        if (!res)
            plugin_log_add( log, "node: failed to replace node\n" );
    }
    else
        plugin_log_add( log, "node: unknown node kind \n" );
}
/**
 * Split this node's edge by creating a new node in the middle. Remember 
 * to preserve the "once a leaf always a leaf" property or f will be wrong.
 * @param v the node in question
 * @param loc the place on the edge after which to split it
 * @param str the text we represent
 * @param log the log to send errors to
 * @return the new internal node
 */
node *node_split( node *v, int loc, UChar *str, plugin_log *log )
{
    // create front edge u leading to internal node v
    int u_len = loc-v->start+1;
    node *u = node_create( v->start, u_len, log );
    // now shorten the following node v
    if ( !node_is_leaf(v) )
        v->len -= u_len;
    // replace v with u in the children of v->parent
    replace_child( v, u, str, log );
    v->start = loc+1;
    // reset parents
    u->parent = v->parent;
    v->parent = u;
    // NB v is the ONLY child of u
    u->c.children = v;
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
void node_clear_next( node *v )
{
    if ( PARENT_LIST(v->parent) )
        v->next = NULL;
}
int node_has_next(node *v )
{
    return v->next !=NULL;
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
    return LEN_MASK&v->len;
}
int node_start( node *v )
{
    return v->start;
}
int node_kind( node *v )
{
    return v->len&KIND_MASK;
}
/**
 * Find a child of an internal node starting with a character
 * @param v the internal node
 * @param c the char to look for
 * @return the child node or NULL
 */
node *node_find_child( node *v, UChar *str, UChar c )
{
    if ( PARENT_LIST(v) )
    {
        v = v->c.children;
        while ( v != NULL && str[v->start] != c )
           v = v->next;
        return v;
    }
    else if ( PARENT_HASH(v) )
    {
        node *u = hashtable_get( v->c.ht, c );
        return u;
    }
    else
        return NULL;
}
/**
 * What is the first char of this node?
 * @param str the string it belongs to
 * @param v the node inquestion
 * @return the first char of the node
 */
UChar node_first_char( node *v, UChar *str )
{
    return str[node_start(v)];
}
/**
 * Get the end index of the string this node points to
 * @param v the node in question
 * @param max the biggest value we can return
 * @return the last index or max
 */
int node_end( node *v, int max )
{
    if ( node_len(v) == INFINITY )
        return max;
    else
        return v->start+node_len(v)-1;
}
/**
 * Debug routine
 * @param str the text
 * @param v the node whose children should be printed to the console
 */
void node_print_children( UChar *str, node *v, plugin_log *log )
{
    node_iterator *iter = node_children( v, log );
    while ( node_iterator_has_next(iter) )
    {
        node *u = node_iterator_next(iter);
        printf("%x ",str[u->start]);
    }
    node_iterator_dispose( iter );
    printf("\n");
}
#ifdef MVD_TEST
static char buffer[SCRATCH_LEN];
static UChar ustr[42];
static char *cstr = "Lorem ipsum dolor sit amet, consectetur";
/**
 * Test a node
 * @param passed VAR param number of passed tests
 * @param failed number of failed tests
 */
void node_test( int *passed, int *failed )
{
    int res = convert_from_encoding( cstr, strlen(cstr), ustr, 42, "utf-8" );
    if ( !res )
    {
        (*failed)++;
        fprintf(stderr,"node: failed to encode utf16 string. "
            "skipping other tests...\n");
        return;
    }
    plugin_log *log = plugin_log_create( buffer );
    if ( log != NULL )
    {
        node *n = node_create( 0, 20, log );
        if ( n != NULL )
        {
            if ( n->start == 0 && n->len == 20 )
                (*passed)++;
            else
            {
                fprintf(stderr,"node: failed to initialise node\n");
                (*failed)++;
            }
            node_dispose( n );
        }
        else
            (*failed)++;
        n = node_create_leaf( 27, log );
        if ( n != NULL )
        {
            if ( n->start == 27 && n->len == INFINITY )
                (*passed)++;
            else
            {
                fprintf(stderr,"node: failed to initialise leaf\n");
                (*failed)++;
            }
            node_dispose( n );
        }
        else
            (*failed)++;
        // test multiple children forcing hashtable representation
        n = node_create( 0, 20, log );
        if ( n != NULL )
        {
            int i;
            srand((unsigned)time(NULL));
            for ( i=0;i<MAX_LIST_CHILDREN;i++ )
            {
                node *o = node_create(rand()%20,rand()%20,log);
                node_add_child( n, o, ustr, log );
            }
            if ( plugin_log_pos(log)!= 0 )
            {
                fprintf(stderr,"node: failed to add %d children\n",
                    MAX_LIST_CHILDREN);
                (*failed)++;
            }
            else
                (*passed)++;
            if ( !PARENT_HASH(n) )
            {
                fprintf(stderr,"node: failed to convert node to hash\n");
                (*failed)++;
            }
            else
                (*passed)++;
            // test iterator
            node_iterator *iter = node_children( n, log );
            if ( iter != NULL )
            {
                while ( node_iterator_has_next(iter) )
                {
                    node *o = node_iterator_next(iter);
                    if ( o != NULL )
                    {
                        UChar uc1 = node_first_char( o, ustr );
                        node *p = node_find_child( n, ustr, uc1 );
                        UChar uc2 = node_first_char( p, ustr );
                        if ( uc2 != uc1 )
                        {
                            (*failed)++;
                            fprintf(stderr,
                                "node: failed to find node for char %c\n",
                                (char)uc1);
                            break;
                        }
                        node_set_link( n, o );
                        node *link = node_link(n);
                        if ( link != o )
                        {
                            (*failed)++;
                            fprintf(stderr, "node: failed to set link\n");
                            break;
                        }
                    }
                    else
                    {
                        (*failed)++;
                        fprintf(stderr,"node: iterator failed\n");
                        break;
                    }
                }
                if ( !node_iterator_has_next(iter) )
                    (*passed)+=2;
                node_iterator_dispose( iter );
            }
            node_dispose( n );
        }
        plugin_log_dispose( log );
    }
}
#endif
