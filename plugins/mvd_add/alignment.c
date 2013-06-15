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
#include "linkpair.h"
#include "alignment.h"
#include "hashmap.h"
#include "match.h"
#include "matcher.h"
#include "utils.h"
#include "dyn_array.h"
#include "benchmark.h"

#define PQUEUE_LIMIT 50
// should be set to the number of (hyperthreaded) processors
#define MAX_THREADS 8
#define NUM_SEGMENTS 12

struct alignment_struct 
{
    // the unaligned pair to align
    pair *p;
    // our suffixtree - computed only once from p
    suffixtree *st;
    // the log to record errors in
    plugin_log *log;
    // next alignment in queue
    alignment *next;
};
/**
 * Create an alignment between a bit of text and the MVD.
 * @param text the text range to align to
 * @param tlen the length of text in UChars
 * @param version the ID of the new version
 * @param log the log to record errors in
 */
alignment *alignment_create( UChar *text, int tlen, int version, 
    plugin_log *log )
{
    alignment *a = calloc( 1, sizeof(alignment) );
    if ( a != NULL )
    {
        bitset *bs = bitset_create();
        if ( bs != NULL )
        {
            bitset_set( bs, version );
            a->p = pair_create_basic( bs, text, tlen );
            a->log = log;
            a->st = suffixtree_create( text, tlen, log );
        }
        if ( bs == NULL || a->p == NULL )
        {
            alignment_dispose(a);
            a = NULL;
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
    if ( a->p != NULL )
        pair_dispose( a->p );
    if ( a->st != NULL )
        suffixtree_dispose( a->st );
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
    *tlen = pair_len( a->p );
    return pair_data(a->p);
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
 */
static void transpose_merge( match *mum, linkpair *start_p, 
    linkpair *end_p, alignment *a )
{
    int i;
    dyn_array *segments = dyn_array_create( NUM_SEGMENTS );
    // 1 is simply the pairs within start_p to end_p inclusive
    // 2,3: break up the match in new version
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
}
/**
 * Merge a MUM directly with the MVD pairs list
 * @param mum the mum to merge
 * @param start_p the first linkpair of the match, starting at offset 0
 * @param end_p the last linkpair of the match, ending at linkpair end
 * @param a the alignment to merge within
 */
static void direct_merge( match *mum, linkpair *start_p, 
    linkpair *end_p, alignment *a )
{
    int v = alignment_version(a);
    do
    {
        pair *p = linkpair_pair(start_p);
        bitset *bs = pair_versions(p);
        bitset_set( bs, v );
        if ( start_p != end_p )
            start_p = linkpair_next( start_p, match_versions(mum) );
    } while ( start_p != end_p );
}
/**
 * Align a single match as part of a sequence of matches
 * @param a the alignment object
 * @param mum the match to align from
 */
static void alignment_merge_one( alignment *a, match *mum )
{
    linkpair *start_p = match_start_link( mum );
    linkpair *end_p = match_end_link( mum );
    int start_pos = match_start_pos( mum );
    int end_pos = match_end_pos( mum );
    // split start and end pairs as required
    if ( start_pos != 0 )
    {
        linkpair_split( start_p, start_pos, a->log );
        start_p = linkpair_right( start_p );
    }
    // wrong: check that it's not at the end
    if ( end_pos>0 && end_pos < pair_len(linkpair_pair(end_p)) )
        linkpair_split( end_p, end_pos, a->log );
    if ( match_transposed(mum,alignment_version(a),alignment_len(a)) )
        transpose_merge( mum, start_p, end_p, a );
    else
        direct_merge( mum, start_p, end_p, a );
}
/**
 * Merge the chosen MUM into the pairs array
 * @param a the alignment to merge into the MVD
 * @param mum the mum - transpose or direct
 * @param pairs the pairs linked list
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 */
static void alignment_merge( alignment *a, match *mum, linkpair *pairs, 
    alignment **left, alignment **right )
{
    // set lhs
    match *prev;
    int v = alignment_version( a );
    if ( match_st_off(mum)>0 )
    {
        int llen = match_st_off(mum);
        *left = alignment_create( pair_data(a->p),llen,v, a->log);
    }
    else
        *left = NULL;
    do
    {
        prev = mum;
        alignment_merge_one( a, mum );
        mum = match_next( mum );
    }
    while ( mum != NULL );
    // set rhs
    if ( match_st_off(prev)+match_len(prev)<pair_len(a->p) )
    {
        int rlen = match_len(prev);
        UChar *pdata = pair_data(a->p);
        *right = alignment_create( &pdata[match_st_off(prev)],rlen,v,a->log);
    }
    else
        *right = NULL;
    // no longer required
    alignment_dispose( a );
}
/**
 * Add the new version at the start of the list of pairs
 * @param a the alignment
 * @param pairs the pairs list
 * @return the new augmented pairs list or NULL
 */
static linkpair *add_to_pairs( alignment *a, linkpair *pairs )
{
    linkpair *lp = NULL;
    lp = linkpair_create( a->p, a->log );
    if ( lp != NULL )
    {
        linkpair_set_right( lp, pairs );
        linkpair_set_left( pairs, lp );
    }
    return lp;
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
int alignment_align( alignment *a, linkpair *pairs, 
    alignment **left, alignment **right, plugin_log *log )
{
    int res = 0;
    pairs = add_to_pairs( a, pairs );
    if ( pairs != NULL )
    {
        aatree *pq = aatree_create( match_compare, PQUEUE_LIMIT );
        if ( pq != NULL )
        {
            matcher *m = matcher_create( a, pq, pairs );
            if ( m != NULL )
            {
                if ( res = matcher_align(m) )
                {
                    match *mum = matcher_get_mum( m );
                    if ( mum != NULL )
                        alignment_merge( a, mum, pairs, left, right );
                }
                matcher_dispose( m );
            }
            aatree_dispose( pq, (aatree_dispose_func)match_dispose );
        }
        else
            plugin_log_add(a->log,
                "alignment: failed to allocate priority queue\n");
    }
    return res == 0;
}
/**
 * Get this alignment's new version
 * @param a the alignment
 * @return its version number for the new version
 */
int alignment_version( alignment *a )
{
    bitset *bs = pair_versions( a->p );
    return bitset_next_set_bit( bs, 1 );
}
/**
 * Get this length of the new version text
 * @param a the alignment
 * @return the length of the new unaligned pair
 */
int alignment_len( alignment *a )
{
    return pair_len(a->p);
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