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
#include "alignment.h"
#include "pos_item.h"
#include "match.h"
#include "matcher.h"

#define PQUEUE_LIMIT 50
// should be set to the number of (hyperthreaded) processors
#define MAX_THREADS 8

struct alignment_struct 
{
    // the new text version to be aligned
    UChar *text;
    // length of new text
    int tlen;
    // start index into pairs
    int start_p;
    // end-index into pairs
    int end_p;
    // our suffixtree - computed only once
    suffixtree *st;
    // next alignment n queue
    alignment *next;
};
/**
 * Create a combination of a bit of text and its directly opposite pairs.
 * @param text the text range to align to
 * @param tlen the length of text in UChars
 * @param start_p index into pairs array for first pair
 * @param end_p index into end of pairs range
 * @param log the log to record errors in
 */
alignment *alignment_create( UChar *text, int tlen, int start_p, int end_p, 
    plugin_log *log )
{
    alignment *a = calloc( 1, sizeof(alignment) );
    if ( a != NULL )
    {
        a->text = text;
        a->st = suffixtree_create( text, tlen, log );
        a->start_p = start_p;
        a->end_p = end_p;
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
 * Append an alignment on the end of another
 * @param tail the tail of the current alignment
 * @param next the new one to add at the end
 */
void alignment_push( alignment *tail, alignment *next )
{
    alignment *temp = tail;
    while ( temp->next != NULL )
        temp = temp->next;
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
 * @return 
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
 * @param mum the mum - transpose or direct
 * @param airs the pairs dyn_array
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 * @param log the log to record errors in
 * @return 1 if it worked, else 0
 */
static int merge( match *mum, dyn_array *pairs, alignment **left, 
    alignment **right, plugin_log *log )
{
    pair *sp = dyn_array_get( pairs, match_start_index(mum) );
    pair *ep = dyn_array_get( pairs, match_end_index(mum) );
    if ( match_start_pos(mum) > 0 )
    {
        pair *ssp = pair_split( sp, match_start_pos(mum) );
}
/**
 * Align with all the OTHER alignments in our list simultaneously
 * @param head the list of other alignments
 * @param pairs VAR param pairs to be updated
 * @param left VAR param set to the leftover alignment on the left
 * @param right VAR param set to the leftover alignment on the right
 * @param log to record errors in
 * @return 1 if it merged correctly, else 0
 */
int alignment_align( alignment *head, dyn_array *pairs, 
    alignment **left, alignment **right, plugin_log *log )
{
    int i,j,res = 0;
    int first = 1;
    pthread_t threads[MAX_THREADS];
    aatree *pq = aatree_create( match_compare, PQUEUE_LIMIT );
    if ( pq != NULL )
    {
        do
        {
            for ( i=0;i<MAX_THREADS,head!=NULL;i++ )
            {
                matcher *m = matcher_create(head->st, pq, head->text, 
                    (pair**)dyn_array_data(pairs), 
                    head->start_p, head->end_p, !first, log );
                first = 0;
                if ( m != NULL )
                {
                    res = pthread_create( &threads[i++], NULL, align_run, m );
                    if ( res )
                    {
                        plugin_log_add( log, 
                            "alignment: failed to create thread: %s\n",
                            strerror(errno));
                        break;
                    }
                }
                else
                    break;
                head = head->next;
            }
            if ( res == 0 )
            {
                // wait for all threads to complete before proceeding
                for ( j=0;j<i;j++ )
                {
                    res = pthread_join( threads[j], NULL );
                    if ( res != 0 )
                    {
                        plugin_log_add(log,
                            "alignment: failed to join thread: %s\n",
                            strerror(errno));
                        break;
                    }
                }
            }
        } while ( head != NULL && res == 0 );
        if ( res == 0 )
        {
            match *mum = get_mum( pq );
            res = merge( mum, pairs, left, right, log );
        }
        aatree_dispose( pq );
    }
    else
        plugin_log_add(log,"alignment: failed to allocate priority queue\n");
    return res == 0;
}
            