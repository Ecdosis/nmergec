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
/* 
 * File:   main.c
 * Author: desmond
 *
 * Created on January 29, 2013, 8:13 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "unicode/uchar.h"
#include "plugin_log.h"
#include "node.h"
#include "path.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#ifdef MVD_TEST
#include <dirent.h>
#include <errno.h>
#include "plugin_log.h"
#include "version.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "hashmap.h"
#include "utils.h"
#endif
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
    v = node_find_child( v, st->str, st->str[start] );
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
            v = node_find_child( v, st->str, st->str[start] );
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
 * Advance a search by one character. 
 * @param st the suffixtree to search
 * @param p the position in the tree of the last match, update if c found
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
        node *n = node_find_child(p->v,st->str,c);
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
        return node_find_child(p->v,st->str,c) != NULL;
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
        node *u = node_find_child( p->v, st->str, st->str[i] );
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
            node_add_child( p->v, leaf, st->str, log );
            update_current_link( st, p->v );
        }
        else
        {
            node *u = node_split( p->v, p->loc, st->str, log );
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
            node_add_child( u, leaf, st->str, log );
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
 * @param log the log to record errors in
 */
static void set_e( suffixtree *st, node *v, plugin_log *log )
{
    if ( node_is_leaf(v) )
    {
        node_set_len( v, st->e-node_start(v)+1 );
    }
    node_iterator *iter = node_children( v, log );
    if ( iter != NULL )
    {
        while ( node_iterator_has_next(iter) )
        {
            node *u = node_iterator_next( iter );
            set_e( st, u, log );
        }
        node_iterator_dispose( iter );
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
    if ( txt[tlen] != 0 )
    {
        plugin_log_add(log,"suffixtree: text not null-terminated!");
        return NULL;
    }
    else
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
                    node_add_child( st->root, st->f, st->str, log );
                    for ( i=1; i<=tlen; i++ )
                        phase(st,i,log);
                    set_e( st, st->root, log );
                }
            }
        }
        else
            fprintf(stderr,"suffixtree: failed to allocate tree\n");
        return st;
    }
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
UChar *suffixtree_text( suffixtree *st )
{
    return st->str;
}
#ifdef MVD_TEST
static char *folder;
typedef struct entry_struct entry;
struct entry_struct
{
    char *file;
    int size;
    long time;
    long space;
    entry *next;
};
static entry *entries=NULL;
static void append_entry( entry *e )
{
    if (entries == NULL )
        entries = e;
    else
    {
        entry *f = entries;
        while ( f->next != NULL )
            f = f->next;
        f->next = e;
    }
}
static void dispose_entries()
{
    entry *e = entries;
    while ( e != NULL )
    {
        entry *next = e->next;
        free( e->file );
        free( e );
        e = next;
    }
}
static char *create_path( char *dir, char *file )
{
    int len1 = strlen( dir );
    int len2 = strlen( file );
    char *path = malloc( len1+len2+2 );
    if ( path != NULL )
    {
        strcpy( path, dir );
        strcat( path, "/" );
        strcat( path, file );
    }
    return path;
}
/**
 * Read a directory, saving the file information in entries
 * @param folder the folder where the files are
 * @param passed VAR param number of passed tests
 * @param failed VAR param number of failed tests
 * @return number of files found or 0 on failure
 */
static int read_dir( char *folder, int *passed, int *failed, plugin_log *log )
{
    int n_files = 0;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(folder)) != NULL) 
    {
        while ((ent = readdir(dir)) != NULL) 
        {
            int flen;
            int old_passed = *passed;
            if ( strcmp(ent->d_name,".")!=0&&strcmp(ent->d_name,"..")!=0 )
            {
                char *path = create_path(folder,ent->d_name);
                //printf("building tree for %s\n",ent->d_name);
                char *txt = read_file( path, &flen );
                if ( txt == NULL )
                    break;
                else
                {
                    int tlen = strlen(txt);
                    long mem2,mem1 = get_mem_usage();
                    int64_t time2,time1 = epoch_time();
                    int ulen = measure_from_encoding( txt, flen, "utf-8" );
                    if ( ulen > 0 )
                    {
                        UChar *dst = calloc( ulen+1, sizeof(UChar) );
                        if ( dst != NULL )
                        {
                            int res = convert_from_encoding( txt, flen, dst, 
                                ulen+1, "utf-8" );
                            if ( res )
                            {
                                suffixtree *tree = suffixtree_create( dst, 
                                    ulen, log );
                                if ( tree != NULL )
                                {
                                    mem2 = get_mem_usage();
                                    time2 = epoch_time();
                                    entry *e = calloc( 1, sizeof(entry) );
                                    if ( e != NULL )
                                    {
                                        e->file = strdup(ent->d_name);
                                        e->space = mem2-mem1;
                                        e->time = time2-time1;
                                        e->size = flen;
                                        append_entry( e );
                                        (*passed)++;
                                        n_files++;
                                    }
                                    else
                                    {
                                        n_files = 0;
                                        dispose_entries();
                                        fprintf(stderr,
                                            "test: failed to allocate entry\n");
                                        break;
                                    }
                                    suffixtree_dispose( tree );
                                }
                            }
                            free(dst);
                        }
                    }
                    free( txt );
                }
                if ( *passed == old_passed )
                {
                    (*failed)++;
                    fprintf(stderr,"suffixtree: failed to create tree %s\n",path);
                }
                if ( path != NULL )
                    free( path );
            }
        }
        closedir( dir );
    }
    else
        fprintf(stderr,"test: failed to open directory %s\n",folder);
    return n_files;
}
// arguments: folder of text files
void suffixtree_test( int *passed, int *failed )
{
    plugin_log *log = plugin_log_create();
    if ( log != NULL )
    {
        int res = read_dir( "tests", passed, failed, log );
        if ( res > 0 )
        {
            entry *e = entries;
            while ( e != NULL )
            {
                //printf("%s\t%d\t%ld\t%ld\n",e->file,e->size,e->time,e->space);
                e = e->next;
            }
            dispose_entries();
        }
    }
    else
    {
        fprintf(stderr,"suffixtree: failed to initialise log\n");
        (*failed)++;
    }
}
#endif
