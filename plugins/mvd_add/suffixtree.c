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
/* 
 * File:   main.c
 * Author: desmond
 *
 * Created on January 29, 2013, 8:13 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "unicode/uchar.h"
#include "plugin_log.h"
#include "node.h"
#include "print_tree.h"
#include "path.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct suffixtree_struct
{
    // the actual string we are building a tree of
    UChar *str;
    // length of str
    size_t slen;
    // end of current leaves
    int e;
    // the root of the tree
    node *root;
    // the first leaf with the longest suffix
    node *f;
    // the last created internal node
    node *current;
    // the last position of str[j..i-1] used in the extension algorithm
    pos last;
    // location of last suffix str[j..i] inserted by an extension
    pos old_beta;
    // the last value of j in the previous extension
    int old_j;
};
/**
 * Find a child of an internal node starting with a character
 * @param st the suffixtree in question
 * @param v the internal node
 * @param c the char to look for
 * @return the child node
 */
static node *find_child( suffixtree *st, node *v, UChar c )
{
    v = node_children(v);
    while ( v != NULL && st->str[node_start(v)] != c )
       v = node_next(v);
    return v;
}
#ifdef DEBUG
#include "debug"
#endif
/**
 * Create a position safely
 * @return the finished pos or fail
 */
static pos *pos_create()
{
    pos *p = calloc( 1, sizeof(pos) );
    if ( p == NULL )
        fprintf( stderr,"couldn't create new pos\n" );
    return p;
}
/**
 * Walk down the tree from the given node following the given path
 * @param st the suffixtree in question
 * @param v the node to start from its children
 * @param p the path to walk down and then free
 * @return a position corresponding to end
 */
static pos *walk_down( suffixtree *st, node *v, path *p )
{
    pos *q=NULL;
    int start = path_start( p );
    int len = path_len( p );
    v = find_child( st, v, st->str[start] );
    while ( len > 0 )
    {
        if ( len <= node_len(v) )
        {
            q = pos_create();
            q->loc = node_start(v)+len-1;
            q->v = v;
            break;
        }
        else
        {
            start += node_len(v);
            len -= node_len(v);
            v = find_child( st, v, st->str[start] );
        }
    }
    path_dispose( p );
    return q;
}
/**
 * Find a location of the suffix in the tree.
 * @param st the suffixtree in question
 * @param j the extension number counting from 0
 * @param i the current phase - 1
 * @param log the log to record errors in
 * @return the position (combined node and edge-offset)
 */ 
static pos *find_beta( suffixtree *st, int j, int i, plugin_log *log )
{
    pos *p;
    if ( st->old_j > 0 && st->old_j == j )
    {
        p = pos_create();
        p->loc = st->old_beta.loc;
        p->v = st->old_beta.v;
    }
    else if ( j>i )  // empty string
    {
        p = pos_create();
        p->loc = 0;
        p->v = st->root;
    }
    else if ( j==0 )    // entire string
    {
        p = pos_create();
        p->loc = i;
        p->v = st->f;
    }
    else // walk across tree
    {
        node *v = st->last.v;
        int len = st->last.loc-node_start(st->last.v)+1;
        path *q = path_create( node_start(v), len, log );
        v = node_parent( v );
        while ( v != st->root && node_link(v)==NULL )
        {
            path_prepend( q, node_len(v) );
            v = node_parent( v );
        }
        if ( v != st->root )
        {
            v = node_link( v );
            p = walk_down( st, v, q );
        }
        else
        {
            path_dispose( q );
            p = walk_down( st, st->root, path_create(j,i-j+1,log) );
        }
    }
    st->last = *p;
    return p;
}
/**
 * Advance a search by one character
 * @param st the suffixtree to search
 * @param p the position in the tree to start from, update if c found
 * @param c the character to find next
 * @return 1 if the next char was found else 0
 */
int suffixtree_advance_pos( suffixtree *st, pos *p, UChar c )
{
    int res = 1;
    if ( node_end(p->v,st->e) > p->loc )
    {
        if ( st->str[p->loc+1] == c )
            p->loc++;
        else
            res = 0;
    }
    else
    {
        node *n = find_child(st,p->v,c);
        if ( n != NULL )
        {
            p->loc = node_start(n);
            p->v = n;
        }
        else
            res = 0;
    }
    return res;
}
/**
 * Does the position continue with the given character?
 * @param st the suffixtree object
 * @param p a position in the tree. 
 * @param c the character to test for in the next position
 * @return 1 if it does else 0
 */
static int continues( suffixtree *st, pos *p, UChar c )
{
    if ( node_end(p->v,st->e) > p->loc )
        return st->str[p->loc+1] == c;
    else
        return find_child(st,p->v,c) != NULL;
}
/**
 * If current is set set its link to point to the next node, then clear it
 * @param st the suffixtree in question
 * @param v the link to point current to
 */
void update_current_link( suffixtree *st, node *v )
{
    if ( st->current != NULL )
    {
        node_set_link( st->current, v );
#ifdef DEBUG
        verify_link( current );
#endif
        st->current = NULL;
    }
}
/**
 * Are we at the end of this edge?
 * @param p the position to test
 * @return 1 if it is, else 0
 */
int pos_at_edge_end( suffixtree *st, pos *p )
{
    return p->loc==node_end(p->v,st->e);
}
/**
 * Record the position where the latest suffix was inserted
 * @param st the suffixtree in question
 * @param p the position of j..i-1.
 * @param i the desired index of the extra char
 */
static void update_old_beta( suffixtree *st, pos *p, int i )
{
    if ( node_end(p->v,st->e) > p->loc )
    {
        st->old_beta.v = p->v;
        st->old_beta.loc = p->loc+1;
    }
    else
    {
        node *u = find_child( st, p->v, st->str[i] );
        st->old_beta.v = u;
        st->old_beta.loc = node_start( u );
    }
}
/**
 * Extend the implicit suffix tree by adding one suffix of the current prefix
 * @param st the current suffixtree
 * @param j the offset into str of the suffix's start
 * @param i the offset into str at the end of the current prefix
 * @param log the log to record errors in
 * @return 1 if the phase continues else 0
 */
static int extension( suffixtree *st, int j, int i, plugin_log *log )
{
    int res = 1;
    pos *p = find_beta( st, j, i-1, log );
    // rule 1 (once a leaf always a leaf)
    if ( node_is_leaf(p->v) && pos_at_edge_end(st,p) )
        res = 1;
    // rule 2
    else if ( !continues(st,p,st->str[i]) )
    {
        //printf("applying rule 2 at j=%d for phase %d\n",j,i);
        node *leaf = node_create_leaf( i, log );
        if ( p->v==st->root || pos_at_edge_end(st,p) )
        {
            node_add_child( p->v, leaf );
            update_current_link( st, p->v );
        }
        else
        {
            node *u = node_split( p->v, p->loc, log );
            update_current_link( st, u );
            if ( i-j==1 )
            {
                node_set_link( u, st->root );
#ifdef DEBUG
                verify_link( current );
#endif
            }
            else 
                st->current = u;
            node_add_child( u, leaf );
        }
        update_old_beta( st, p, i );
    }
    // rule 3
    else
    {
        //printf("applying rule 3 at j=%d for phase %d\n",j,i);
        update_current_link( st, p->v );
        update_old_beta( st, p, i );
        res = 0;
    }
    free( p );
    return res;
}
/**
 * Process the prefix of str ending in the given offset
 * @param st the current suffixtree
 * @param i the inclusive end-offset of the prefix
 * @param log the log to record errors in
 */
static void phase( suffixtree *st, int i, plugin_log *log )
{
    int j;
    st->current = NULL;
    for ( j=st->old_j;j<=i;j++ )            
        if ( !extension(st,j,i,log) )
            break;
    // remember number of last extension for next phase
    st->old_j = (j>i)?i:j;
    // update all leaf ends
    st->e++;
    //print_tree( root );
}
/**
 * Set the length of each leaf to e recursively
 * @param v the node in question
 */
static void set_e( suffixtree *st, node *v )
{
    if ( node_is_leaf(v) )
    {
        node_set_len( v, st->e-node_start(v)+1 );
    }
    node *u = node_children( v );
    while ( u != NULL )
    {
        set_e( st, u );
        u = node_next( u );
    }
}
/**
 * Build a tree using a given string
 * @param txt the text to build it from
 * @param tlen its length
 * @param log the log to record errors in
 * @return the finished tree
 */
suffixtree *suffixtree_create( UChar *txt, size_t tlen, plugin_log *log )
{
    suffixtree *st = calloc( 1, sizeof(suffixtree) );
    if ( st != NULL )
    {
        st->e = 0;
        memset( &st->last, 0, sizeof(pos) );
        memset( &st->old_beta, 0, sizeof(pos) );
        st->str = txt;
        st->slen = tlen;
        // actually build the tree
        st->root = node_create( 0, 0, log );
        if ( st->root != NULL )
        {
            st->f = node_create_leaf( 0, log );
            if ( st->f != NULL )
            {
                int i;
                node_add_child( st->root, st->f );
                for ( i=1; i<=tlen; i++ )
                    phase(st,i,log);
                set_e( st, st->root );
            }
        }
    }
    else
        fprintf(stderr,"suffixtree: failed to allocate tree\n");
    return st;
}
/**
 * Dispose of a suffix tree. We don't own the text
 * @param st the suffixtree to dispose
 */
void suffixtree_dispose( suffixtree *st )
{
    node_dispose( st->root );
    free( st );
}
/**
 * Get the root node
 * @param st the suffixtree object
 * @return the node of the root
 */
node *suffixtree_root( suffixtree *st )
{
    return st->root;
}