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
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "aatree.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "linkpair.h"
#include "orphanage.h"
#include "alignment.h"
#include "hashmap.h"
#include "match.h"
#include "matcher.h"
#include "utils.h"
#include "benchmark.h"

// should be set to the number of (hyperthreaded) processors
#define MAX_THREADS 8
#define NUM_SEGMENTS 12
UChar USTR_EMPTY[] = {0};

struct alignment_struct 
{
    // our version
    int version;
    // our text
    UChar *text;
    // offset in text where we start
    int start;
    // length of text in characters
    int tlen;
    // our suffixtree - computed only once from p
    suffixtree *st;
    // register or children and parents
    orphanage *o;
    // the log to record errors in
    plugin_log *log;
    // next alignment in queue
    alignment *next;
};
/**
 * Create an alignment between a bit of text and the MVD.
 * @param text the text of the whole new version
 * @param start the start offset in text
 * @param tlen the length of text in UChars
 * @param version the ID of the new version
 * @param log the log to record errors in
 */
alignment *alignment_create( UChar *text, int start, int tlen, int version, 
    orphanage *o, plugin_log *log )
{
    alignment *a = calloc( 1, sizeof(alignment) );
    if ( a != NULL )
    {
        bitset *bs = bitset_create();
        if ( bs != NULL )
        {
            a->text = text;
            a->start = start;
            a->tlen = tlen;
            a->version = version;
            a->log = log;
            a->o = o;
            a->st = suffixtree_create( &text[start], tlen, log );
            if ( a->st == NULL )
            {
                alignment_dispose( a );
                plugin_log_add(log,"alignment: failed to create suffix tree\n");
                a = NULL;
            }
        }
    }
    else
        plugin_log_add( log, "alignment: failed to allocate object\n");
    return a;
}
/**
 * Dispose of an alignment.
 * @param a the alignment to dispose
 */
void alignment_dispose( alignment *a )
{

    if ( a->st != NULL )
        suffixtree_dispose( a->st );
    // text belongs to the caller
    free( a );
}
/**
 * Get the textual component of this alignment
 * @param a the alignment object
 * @param tlen VAR param: its length
 * @return the text in UTF-16 format
 */
UChar *alignment_text( alignment *a, int *tlen )
{
    *tlen = a->tlen;
    return &a->text[a->start];
}
/**
 * Get the log for this alignment
 * @param a the alignment
 * @return a plugin log
 */
plugin_log *alignment_log( alignment *a )
{
    return a->log;
}
/**
 * Append an alignment on the end of another. Keep sorted on decreasing length.
 * @param tail the tail of the current alignment
 * @param next the new one to add at the end
 */
void alignment_push( alignment *head, alignment *next )
{
    alignment *prev = NULL;
    alignment *temp = head;
    while ( temp->next != NULL && alignment_len(temp) > alignment_len(next) )
    {
        prev = temp;
        temp = temp->next;
    }
    if ( prev != NULL )
        prev->next = temp;
    temp->next = next;
}
/**
 * Remove from the head of the list
 * @param head the list start
 * @return the new head
 */
alignment *alignment_pop( alignment *head )
{
    alignment *rest = head->next;
    head->next = NULL;
    return rest;
}
/**
 * Merge a transposed MUM into the MVD pairs list
 * @param mum the mum to merge
 * @param a the alignment in question
 */
static void alignment_transpose_merge( alignment *a, match *mum )
{
    do
    {
        linkpair *temp = match_start_link( mum );
        linkpair *end = match_end_link( mum );
        bitset *mv = match_versions( mum );
        int st_off = match_st_off( mum );
        do
        {
            linkpair *parent = linkpair_make_parent( temp, a->log );
            if ( parent == NULL && pair_is_child(linkpair_pair(temp)) )
                parent = orphanage_get_parent(a->o,temp);
            if ( parent != NULL )
            {
                pair_set_id( linkpair_pair(parent), orphanage_next_id(a->o) );
                orphanage_add_parent( a->o, parent );
                linkpair *child = linkpair_make_child( parent, a->version, 
                    a->log );
                pair_set_id( linkpair_pair(child), orphanage_next_id(a->o) );
                linkpair_set_st_off( child, st_off );
                orphanage_add_child( a->o, child );
                pair *pp = linkpair_pair(parent);
                st_off += pair_len(pp);
            }
            else
                plugin_log_add(a->log,"alignment: failed to create parent\n");
            temp = linkpair_next( temp, mv );
        } while ( temp != end );
        mum = match_next( mum );
    } while ( mum != NULL );
}
/**
 * Merge a MUM directly with the MVD pairs list
 * @param mum the mum to merge
 * @param start_p the first linkpair of the match, starting at offset 0
 * @param end_p the last linkpair of the match, ending at linkpair end
 * @param a the alignment to merge within
 */
static void alignment_direct_merge( alignment *a, match *mum )
{
    do
    {
        linkpair *temp = match_start_link( mum );
        linkpair *end_p = match_end_link( mum );
        bitset *mv = match_versions(mum);
        // align the middle bit
        int v = alignment_version( a );
        do
        {
            pair *p = linkpair_pair(temp);
            bitset *bs = pair_versions(p);
            bitset_set( bs, v );
            if ( temp != end_p )
                temp = linkpair_next( temp, mv );
        } while ( temp != end_p );
        mum = match_next( mum );
    } while ( mum != NULL );
}
/**
 * Split the left-hand-side off from the merged alignment
 * @param a the alignment whose mum has been computed
 * @param mum the mum in question
 * @param left update with the new left-hand alignment if any or NULL
 * @return 1 if it worked
 */
static int alignment_create_lhs( alignment *a, match *mum, alignment **left )
{
    int res = 1;
    int v = alignment_version( a );
    if ( match_st_off(mum)>0 )
    {
        int llen = match_st_off(mum);
        *left = alignment_create( a->text, a->start, llen, v, a->o, a->log );
        if ( *left == NULL )
        {
            plugin_log_add(a->log,"alignment: failed to create lhs\n");
            res = 0;
        }
    }
    else
        *left = NULL;
    return res;
}
/**
 * Split off the rhs of a match
 * @param a the alignment
 * @param last the last mum in the sequence (if several)
 * @return 1 if it worked else 0
 */
int alignment_create_rhs( alignment *a, match *last, alignment **right )
{
    int res = 1;
    int v = alignment_version( a );
    if ( match_st_end(last) < a->tlen )
    {
        int rlen = a->tlen-match_st_end(last);
        *right = alignment_create( a->text,match_st_end(last),rlen,v,a->o,a->log);
        if ( *right == NULL )
        {
            plugin_log_add(a->log,"alignment: failed to create lhs\n");
            res = 0;
        }
    }
    else 
        *right = NULL;  
    return res;
}
/**
 * Create a leftover pair on the left and add it to the pairs list
 * @param a the alignment in question
 * @param mum the maximal unique match
 */
static void alignment_add_newpair_left( alignment *a, match *mum )
{
    linkpair *llp = NULL;
    UChar *new_text = NULL;
    int llen = match_st_off(mum);
    if ( llen > 0 )
        new_text = &a->text[a->start];
    else
    {
        linkpair *lp = match_start_link(mum);
        if ( linkpair_left(lp) != NULL )
            new_text = USTR_EMPTY;
    }
    if ( new_text != NULL )
    {
        int res = 0;
        bitset *bs = bitset_create();
        bitset_set( bs, a->version );
        pair *p = pair_create_basic( bs, new_text, llen );
        llp = linkpair_create( p, a->log );
        linkpair *lp_start = match_start_link(mum);
        linkpair_add_before( lp_start, llp ); 
    }
}
/**
 * Add a new pair based on the new version at the end 
 * @param a the alignment
 * @param mum the match AFTER which to insert the linkpair
 */
static void add_newpair_right( alignment *a, match *mum )
{
    linkpair *rlp = NULL;
    UChar *new_text = NULL;
    int rlen = alignment_len(a)-match_st_end(mum);
    if ( rlen > 1 ) // inclusive end
        new_text = &a->text[match_st_off(mum)];
    else
    {
        linkpair *lp = match_end_link(mum);
        if ( linkpair_right(lp) != NULL )
            new_text = USTR_EMPTY;
    }
    if ( new_text != NULL )
    {
        int res = 0;
        bitset *bs = bitset_create();
        bitset_set( bs, a->version );
        pair *p = pair_create_basic( bs, new_text, rlen );
        rlp = linkpair_create( p, a->log );
        linkpair *lp_end = match_end_link(mum);
        linkpair_add_after( lp_end, rlp ); 
    }
} 
/**
 * Merge the chosen MUM into the pairs array
 * @param a the alignment to merge into the MVD
 * @param mum the mum - transpose or direct
 * @param pairs the pairs linked list: update it!
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 * @return 1 if it worked
 */
static int alignment_merge( alignment *a, match *mum, linkpair **pairs, 
    alignment **left, alignment **right )
{
    int res = 1;
    res = alignment_create_lhs( a, mum, left );
    if ( res )
    {
        match *last = mum;
        res = match_split( mum, a->text, a->version, a->log );
        if ( res )
        {
            alignment_add_newpair_left( a, mum );
            if ( match_transposed(mum,alignment_version(a),alignment_len(a)) )
                alignment_transpose_merge( a, mum );
            else
                alignment_direct_merge( a, mum );
            while ( match_next(last) != NULL )
                last = match_next(last);
            add_newpair_right( a, last );
            res = alignment_create_rhs( a, last, right );
            if ( !res )
                plugin_log_add(a->log,"alignment: failed to create rhs\n");
        }
        else
            plugin_log_add(a->log,"alignment: failed to split match\n");
    }
    else
        plugin_log_add(a->log,"alignment: failed to create lhs\n");
    alignment_dispose( a );
    return res;
}
/**
 * Align with the whole text excluding those segments already aligned
 * @param head the list of alignments
 * @param pairs the list of pairs to be searched
 * @param left VAR param set to the leftover alignment on the left
 * @param right VAR param set to the leftover alignment on the right
 * @param log the log to record errors in
 * @return 1 if it merged correctly, else 0
 */
int alignment_align( alignment *a, linkpair **pairs, 
    alignment **left, alignment **right, orphanage *o, plugin_log *log )
{
    int res = 0;
    matcher *m = matcher_create( a, *pairs );
    if ( m != NULL )
    {
        if ( res = matcher_align(m) )
        {
            match *mum = matcher_get_mum( m );
            if ( mum != NULL )
                res = alignment_merge( a, mum, pairs, left, right );
        }
        matcher_dispose( m );
    }
    else
        res = 0;
    return res;
}
/**
 * Get this alignment's new version
 * @param a the alignment
 * @return its version number for the new version
 */
int alignment_version( alignment *a )
{
    return a->version;
}
/**
 * Get this length of the new version text
 * @param a the alignment
 * @return the length of the new unaligned pair
 */
int alignment_len( alignment *a )
{
    return a->tlen;
}
/**
 * Get this alignment's suffixtree
 * @param a the laignment in question
 * @return a suffixtree made of this alignment's text
 */
suffixtree *alignment_suffixtree( alignment *a )
{
    return a->st;
}