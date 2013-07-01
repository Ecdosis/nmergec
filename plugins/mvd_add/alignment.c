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
#include "alignment.h"
#include "hashmap.h"
#include "match.h"
#include "matcher.h"
#include "utils.h"
#include "benchmark.h"
#include "linkpair.h"

#define PQUEUE_LIMIT 50
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
    // our linkpair, composed from the above
    linkpair *lp;
    // our suffixtree - computed only once from p
    suffixtree *st;
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
    plugin_log *log )
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
            a->st = suffixtree_create( text, tlen, log );
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bitset_set( bs, version );
                pair *p = pair_create_basic( bs, text, tlen );
                if ( p != NULL )
                    a->lp = linkpair_create( p, log );
                bitset_dispose( bs );
            }
            if ( a->lp == NULL )
            {
                alignment_dispose( a );
                plugin_log_add(log,"alignment: failed to initialise object\n");
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
    if ( a->lp != NULL )
    {
        if ( linkpair_pair(a->lp) != NULL )
            pair_dispose( linkpair_pair(a->lp) );
        linkpair_dispose( a->lp );
    }
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
 * Get the linkpair associated with this alignment
 * @param a the alignment in question
 * @return the linkpair
 */
linkpair *alignment_linkpair( alignment *a )
{
    return a->lp;
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
 * Merge a MUM into the list of pairs by transposition
 * 1. determine which pairs in the existing MVD are to be aligned with
 * 2. break up the new version into corresponding segments
 * 3. make the MVD pairs into parents if they are not already (or align 
 * with parents of children)
 * 4. convert new segments into linkpairs and merge into the list of pairs
 * 5. repair the effects of adding the new pairs
 * @param mum the mum to merge
 * @param start_p the first linkpair that fully matches
 * @param end_p the last linkpair that fully matches
 * @param a the alignment in question
 * @return 1 if it worked
 */
static int alignment_transpose_merge( alignment *a, match *mum )
{
    int i;
    int res = 1;
    dyn_array *segments = dyn_array_create( NUM_SEGMENTS );
    // 1 is simply the pairs within start_p to end_p inclusive
    // 2,3: break up the match in new version
    linkpair *start_p = match_start_link( mum );
    linkpair *end_p = match_end_link( mum );
    linkpair *lp = start_p;
    do
    {
        pair *p = linkpair_pair( lp );
        bitset *bs = bitset_create();
        bitset_set( bs, alignment_version(a) );
        pair *q = pair_create_child( bs );
        if ( pair_is_child(p) )
            p = pair_parent(p);
        pair_add_child( p, q );
        dyn_array_add( segments, q );
        if ( lp != end_p )
            lp = linkpair_next( lp, match_versions(mum) );
    } while ( lp != end_p );
    // 4. convert into linkpairs
    linkpair *head = NULL;
    linkpair *prev = NULL;
    for ( i=0;i<dyn_array_size(segments);i++ )
    {
        pair *r = (pair*)dyn_array_get(segments,i);
        lp = linkpair_create( r, a->log );
        if ( prev != NULL )
        {
            linkpair_set_left( lp, prev );
            linkpair_set_right( prev, lp );
        }
        else
            head = lp;
        prev = lp;
    }
    // merge into pairs list
    prev = lp;
    lp = linkpair_right( start_p );
    while ( !linkpair_free(lp) )
    {
        prev = lp;
        lp = linkpair_right(lp);
    }
    if ( lp == NULL )
        linkpair_set_right( prev, head );
    else
    {
        linkpair *tail = lp;
        while ( linkpair_right(tail) != NULL )
            tail = linkpair_right(tail);
        linkpair_set_left(lp,tail);
        linkpair_set_right(tail,lp);
        linkpair_set_left(head,linkpair_left(lp));
        linkpair_set_right(linkpair_right(lp),head);
        linkpair_add_hint( start_p, alignment_version(a), a->log );
    }
    // make this have meaning later
    return res;
}
/**
 * Merge a MUM directly with the MVD pairs list
 * @param mum the mum to merge
 * @param start_p the first linkpair of the match, starting at offset 0
 * @param end_p the last linkpair of the match, ending at linkpair end
 * @param a the alignment to merge within
 */
static int alignment_direct_merge( alignment *a, match *mum )
{
    int res = 1;
    do
    {
        linkpair *start_p = match_start_link( mum );
        linkpair *end_p = match_end_link( mum );
        // align the middle bit
        int v = alignment_version( a );
        do
        {
            pair *p = linkpair_pair(start_p);
            bitset *bs = pair_versions(p);
            if ( bs != NULL )
            {
                bitset_set( bs, v );
                if ( start_p != end_p )
                    start_p = linkpair_next( start_p, match_versions(mum) );
            }
            else
                res = 0;
        } while ( res && start_p != end_p );
        if ( res )
            mum = match_next( mum );
    } while ( res && mum != NULL );
    return res;
}
/**
 * Align a single match (and any joined to it)
 * @param a the alignment object
 * @param mum the match to align from
 * @return 1 if it worked
 */
static int alignment_merge_one( alignment *a, match *mum )
{
    int res = 1;
    match_split( mum, a->text, a->version, a->log );
    if ( match_transposed(mum,alignment_version(a),alignment_len(a)) )
        res = alignment_transpose_merge( a, mum );
    else
        res = alignment_direct_merge( a, mum );
    return res;
}
/**
 * Split the left-hand-side off from the merged alignment
 * @param a the alignment whose mum has been computed
 * @param mum the mum in question
 * @param pairs the overall pairs list, perhaps to update
 * @param left update with the new left-hand alignment if any or NULL
 * @return 1 if it worked
 */
static int alignment_create_lhs( alignment *a, match *mum, 
    linkpair **pairs, alignment **left )
{
    int res = 0;
    int v = alignment_version( a );
    if ( match_st_off(mum)>0 )
    {
        int llen = match_st_off(mum);
        *left = alignment_create( a->text, a->start,llen,v, a->log);
        if ( *left != NULL )
        {
            // replace the old linkpair with the new lhs
            linkpair *old = alignment_linkpair( a );
            linkpair *lhs = alignment_linkpair( *left );
            linkpair_replace( old, lhs );
            if ( old == *pairs )
                *pairs = lhs;
            res = 1;
        }
    }
    else // create an empty linkpair but no alignment
    {
        linkpair *old = alignment_linkpair( a );
        if ( linkpair_left(old) != NULL )
        {
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bitset_set( bs, alignment_version(a));
                pair *p = pair_create_basic( bs, USTR_EMPTY, 0 );
                if ( p != NULL )
                {
                    linkpair *lhs = linkpair_create( p, a->log );
                    if ( lhs != NULL )
                    {
                        linkpair_replace( old, lhs );
                        if ( old == *pairs )
                            *pairs = lhs;
                        res = 1;
                    }
                    else
                        pair_dispose( p );
                }
                else
                    bitset_dispose( bs );
            }
        }
        *left = NULL;
    }
    if ( !res )
        plugin_log_add(a->log,"alignment: failed to create lhs\n");
    return res;
}
/**
 * Split off the rhs of a match
 * @param a the alignment
 * @param last the last mum in the sequence (if several)
 * @return 1 if it worked else 0
 */
int alignment_create_rhs( alignment *a, match *last, linkpair *last_merged, 
    alignment **right )
{
    int res = 1;
    int v = alignment_version( a );
    if ( match_st_end(last)<a->tlen )
    {
        int rlen = a->tlen-match_st_end(last);
        *right = alignment_create( a->text,match_st_end(last),rlen,v,a->log);
        if ( *right == NULL )
            res = 0;
    }
    else 
    {
        /*linkpair *old = alignment_linkpair( a );
        if ( linkpair_right(old) != NULL )
        {
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bitset_set( bs, alignment_version(a));
                pair *p = pair_create_basic( bs, USTR_EMPTY, 0 );
                if ( p != NULL )
                {
                    linkpair *rhs = linkpair_create( p, a->log );
                    if ( rhs != NULL )
                        res = linkpair_add_after( last_merged, rhs );
                    else
                        pair_dispose( p );
                }
                else
                    bitset_dispose( bs );
            }
        }*/
        *right = NULL;
    }   
    return res;
}
/**
 * Merge the chosen MUM into the pairs array
 * @param a the alignment to merge into the MVD
 * @param mum the mum - transpose or direct
 * @param pairs the pairs linked list
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 * @return 1 if it worked
 */
static int alignment_merge( alignment *a, match *mum, linkpair **pairs, 
    alignment **left, alignment **right )
{
    int res = 1;
    res = alignment_create_lhs( a, mum, pairs, left );
    if ( res )
    {
        match *last = mum;
        res = alignment_merge_one( a, mum );
        if ( res )
        {
            while ( match_next(last) != NULL )
                last = match_next(last);
            res = alignment_create_rhs( a, last, match_end_link(last), right );
        }
    }
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
    alignment **left, alignment **right, plugin_log *log )
{
    int res = 0;
    aatree *pq = aatree_create( match_compare, PQUEUE_LIMIT );
    if ( pq != NULL )
    {
        matcher *m = matcher_create( a, pq, *pairs );
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
        aatree_dispose( pq, (aatree_dispose_func)match_dispose );
    }
    else
        plugin_log_add(a->log,
            "alignment: failed to allocate priority queue\n");
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