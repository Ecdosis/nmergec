#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
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

#define PQUEUE_LIMIT 50
// should be set to the number of (hyperthreaded) processors
#define MAX_THREADS 8

struct alignment_struct 
{
    // the new text version to be aligned
    UChar *text;
    // length of new text
    int tlen;
    // id of new version
    int version;
    // our suffixtree - computed only once
    suffixtree *st;
    // next alignment n queue
    alignment *next;
};
/**
 * Create a combination of a bit of text and its directly opposite pairs.
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
        a->text = text;
        a->version = version;
        a->st = suffixtree_create( text, tlen, log );
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
    return a->text;
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
    while ( temp->next != NULL && temp->tlen > next->tlen )
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
 * Run an individual alignment
 * @param data generic pointer to matcher object
 * @return NULL
 */
static void *align_run( void *data )
{
    matcher *m = (matcher*)data;
    return NULL;
}
/**
 * Get the longest maximal unique match (MUM)
 * @param m the matcher in question
 * @return a match, which may be a chain of matches
 */
static match *get_mum( aatree *pq )
{
    match *found = NULL;
    while ( !aatree_empty(pq) )
    {
        match *mt = aatree_max( pq );
        if ( match_freq(mt) == 1 )
        {
            found = mt;
            break;
        }
        else if ( !aatree_delete(pq,mt) )
            break;
    }
    return found;
}
/**
 * Merge the chosen MUM into the pairs array
 * @param a the alignment to merge into the MVD
 * @param mum the mum - transpose or direct
 * @param pairs the pairs linked list
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 * @param log the log to record errors in
 * @return 1 if it worked, else 0
 */
static int alignment_merge( alignment *a, match *mum, linkpair *pairs, 
    alignment **left, alignment **right, plugin_log *log )
{
    linkpair *start_p = match_start_link( mum );
    linkpair *end_p = match_end_link( mum );
    int start_pos = match_start_pos( mum );
    int end_pos = match_end_pos( mum );
    if ( start_pos != 0 )
    {
        linkpair_split( start_p, start_pos, log );
        start_p = linkpair_right( start_p );
    }
    if ( end_pos != 0 )
        linkpair_split( end_p, end_pos, log );
    // this is direct align onlyss
    do
    {
        pair *p = linkpair_pair(start_p);
        bitset *bs = pair_versions(p);
        bitset_set( bs, a->version );
        start_p = linkpair_next( start_p, match_versions(mum) );
    } while ( start_p != end_p );
    if ( match_st_off(mum)>0 )
    {
        int llen = match_st_off(mum);
        *left = alignment_create( u_strndup(a->text,llen),llen,a->version,log);
    }
    else
        *left = NULL;
    if ( match_st_off(mum)+match_len(mum)<a->tlen )
    {
        int rlen = match_len(mum);
        *right = alignment_create( u_strndup(&a->text[match_st_off(mum)],rlen),
            rlen,a->version,log);
    }
    else
        *right = NULL;
}
/**
 * Align with the whole text excluding those segments already aligned
 * @param head the list of alignments
 * @param pairs the list of pairs to be searched
 * @param left VAR param set to the leftover alignment on the left
 * @param right VAR param set to the leftover alignment on the right
 * @param log to record errors in
 * @return 1 if it merged correctly, else 0
 */
int alignment_align( alignment *a, linkpair *pairs, 
    alignment **left, alignment **right, plugin_log *log )
{
    int res = 0;
    aatree *pq = aatree_create( match_compare, PQUEUE_LIMIT );
    if ( pq != NULL )
    {
        matcher *m = matcher_create( a->st, pq, a->text, pairs, log );
        if ( res = matcher_align(m) )
        {
            match *mum = get_mum( pq );
            if ( mum != NULL )
                res = alignment_merge( a, mum, pairs, left, right, log );
        }
        aatree_dispose( pq );
    }
    else
        plugin_log_add(log,"alignment: failed to allocate priority queue\n");
    return res == 0;
}
            