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
#include <unicode/ustring.h>
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "aatree.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "card.h"
#include "orphanage.h"
#include "location.h"
#include "match.h"
#include "alignment.h"
#include "hashmap.h"
#include "deck.h"
#include "utils.h"
#include "benchmark.h"
#include "mum.h"

// not used
#define MAX_THREADS 8
#define NUM_SEGMENTS 12
UChar USTR_EMPTY[] = {0};

/**
 * A alignment is a "deal" of part of the new version to match against the deck
 */
struct alignment_struct 
{
    // our version
    int version;
    // our text
    UChar *text;
    // offset in text where we start
    int start;
    /** length of our section of the text */
    int len;
    // overall length of text in characters
    int tlen;
    // our suffixtree - computed only once from p
    suffixtree *st;
    // register or children and parents
    orphanage *o;
    // the log to record errors in
    plugin_log *log;
    // next alignment in queue
    alignment *next;
    // best match found
    mum *best;
    // set to 1 if other mums have been merged that might affect us
    int stale;
};
/**
 * Create an alignment between a bit of text and the MVD.
 * @param text the text of the whole new version in UTF-16
 * @param start the start offset in text
 * @param tlen the length of text in UChars
 * @param version the ID of the new version
 * @param o the orphanage to store unattached children in
 * @param log the log to record errors in
 */
alignment *alignment_create( UChar *text, int start, int len, int tlen, 
    int version, orphanage *o, plugin_log *log )
{
    alignment *a = calloc( 1, sizeof(alignment) );
    if ( a != NULL )
    {
        a->text = text;
        a->start = start;
        a->tlen = tlen;
        a->len = len;
        a->version = version;
        a->log = log;
        a->o = o;
        a->st = suffixtree_create( &text[start], len, log );
        if ( a->st == NULL )
        {
            alignment_dispose( a );
            plugin_log_add(log,"alignment: failed to create suffix tree\n");
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
    if ( a->st != NULL )
        suffixtree_dispose( a->st );
    // text and orphanage belongs to the caller
    free( a );
}
/**
 * Set the stale flag: update at next opportunity
 * @param a the alignment to make stale
 * @param stale the new value of stale flag
 */
void alignment_set_stale( alignment *a, int stale )
{
    a->stale = stale;
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
 * Get the log for this alignment
 * @param a the alignment
 * @return a plugin log
 */
plugin_log *alignment_log( alignment *a )
{
    return a->log;
}
/**
 * Append an alignment on the end of another. Sort on decreasing length of mum.
 * @param head the head of the current alignment
 * @param next the new one to add at the end
 * @return the new or old head (unchanged)
 */
alignment *alignment_push( alignment *head, alignment *next )
{
    alignment *prev = NULL;
    alignment *temp = head;
/*
    printf("pushing %d to %d\n",mum_text_off(next->best),
        mum_text_off(next->best)+mum_len(next->best) );
*/
    while ( temp != NULL && mum_total_len(temp->best) > mum_total_len(next->best) )
    {
        prev = temp;
        temp = temp->next;
    }
    if ( prev != NULL )
    {
        prev->next = next;
        next->next = temp;
    }
    else
    {
        next->next = head;
        head = next;
    }
    /*
    // verify list
    temp = head;
    prev = NULL;
    while ( temp != NULL )
    {
        prev = temp;
        temp = temp->next;
        if ( prev != NULL && temp != NULL && temp->len > prev->len )
            plugin_log_add(head->log,"alignment: error in alignment list!\n");
    }*/
    return head;
}
/**
 * Remove from the head of the list
 * @param head the list start
 * @return the new head
 */
alignment *alignment_pop( alignment *head )
{
    return head->next;
}
/**
 * Debug routine
 * @param a the alignment to print
 * @param prompt the prefix to the alignment
 */
void alignment_print( alignment *a, const char *prompt )
{
    int end = a->start+a->len-1;
    printf( "%s v=%d %d-%d\n", prompt,a->version,a->start,end);
}
/**
 * Merge a transposed MUM into the MVD pairs list. Match is already split.
 * @param a the alignment in question
 * @param discards a set of discarded cards to be add in later
 * @return 1 if it worked else 0
 */
static int alignment_transpose_merge( alignment *a, dyn_array *discards )
{
    int res = 1;
    mum *best = a->best;
    do
    {
        card *temp = mum_start_card( best );
        card *end = mum_end_card( best );
        bitset *mv = mum_versions( best );
        int text_off = mum_text_off( best );
        do
        {
/*
            printf("transpose merging %d to %d\n",mum_text_off(best),mum_len(best)+mum_text_off(best));
*/
            // make every pair in the match a parent:
            // either already a parent, or an ordinary pair or a child
            // this may free temp
            int temp_at_end = temp==end;
            card *parent = card_make_parent( &temp, a->log );
            if ( temp_at_end && temp != end )
                end = temp;
            if ( parent == NULL && pair_is_child(card_pair(temp)) )
                parent = orphanage_get_parent(a->o,temp);
            if ( parent != NULL )
            {
                pair *ppair = card_pair(parent);
                int id = pair_id( ppair );
                id = (id == 0)?orphanage_next_id(a->o):id;
                pair_set_id( card_pair(parent), id );
                orphanage_add_parent( a->o, parent );
                card *child = card_make_child( parent, a->version, 
                    a->log );
                pair_set_id( card_pair(child), id );
                card_set_text_off( child, text_off );
                // add the child later at the end
                res = orphanage_add_child( a->o, child );
                text_off += pair_len(ppair);
                // add blank to cover the new version
                card *blank = card_create_blank( a->version, a->log );
                if ( blank != NULL )
                {
                    pair *p = card_pair(blank);
                    card *prev = card_prev( parent, pair_versions(p) );
                    card_set_text_off( blank, card_end(prev) );
                    dyn_array_add( discards, blank );
                }
                else
                {
                    plugin_log_add(a->log,"alignment: failed to create blank");
                    res = 0;
                    break;
                }
            }
            else // none of those
                plugin_log_add(a->log,"alignment: failed to create parent\n");
            if ( temp != end )
                temp = card_next( temp, mv );
        } while ( temp != end );
        best = mum_next( best );
    } while ( best != NULL );
    return res;
}
/**
 * Merge a MUM directly into the MVD pairs list
 * @param a the alignment to merge within
 * @return 1 if it worked else 0
 */
static int alignment_direct_merge( alignment *a )
{
    int res = 1;
    mum *best = a->best;
    do
    {
        card *temp = mum_start_card( best );
        card *end_p = mum_end_card( best );
        bitset *mv = mum_versions(best);
        int text_off = mum_text_off(best);
        // align the middle bit
        int v = a->version;
        do
        {
            pair *p = card_pair(temp);
            bitset *bs = pair_versions(p);
/*
            printf("directly merging %d to %d\n",mum_text_off(best),mum_len(best)+mum_text_off(best));
*/
            bitset_set( bs, v );
            card_set_text_off( temp, text_off );
            text_off += pair_len(p);
            if ( temp != end_p )
                temp = card_next( temp, mv );
        } while ( temp != end_p );
        best = mum_next( best );
    } while ( best != NULL );
    return res;
}
/**
 * Split the left-alignment-side off from the merged alignment
 * @param a the alignment whose mum has been computed
 * @param left update with the new left-alignment alignment if any or NULL
 * @return 1 if it worked
 */
static int alignment_create_lhs( alignment *a, alignment **left )
{
    int res = 1;
    int v = a->version;
    if ( mum_text_off(a->best)>a->start )
    {
        int llen = mum_text_off(a->best)-a->start;
        *left = alignment_create( a->text, a->start, llen, a->tlen, v, 
            a->o, a->log );
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
static int alignment_create_rhs( alignment *a, mum *last, alignment **right )
{
    int res = 1;
    int v = a->version;
    if ( mum_text_end(last) < alignment_end(a) )
    {
        int rlen = alignment_end(a)-mum_text_end(last);
        *right = alignment_create( a->text,mum_text_end(last),rlen,a->tlen,
            v,a->o,a->log);
        if ( *right == NULL )
        {
            plugin_log_add(a->log,"alignment: failed to create lhs\n");
            res = 0;
        }
    }
    else // not an error
        *right = NULL;  
    return res;
}
/**
 * Align the new version with the deck
 * @param a the alignment to use
 * @param list a list of all the cards
 * @return 1 if it worked else 0
 */
int alignment_align( alignment *a, card *list )
{
    int res = 0;
    deck *d = deck_create( a, list );
    if ( d != NULL )
    {
        res = deck_align( d );
        if ( res )
        {
            match *m = deck_get_mum(d);
            if ( m != NULL )
            {
                a->best = mum_create( m, a->log );
                res = mum_set( a->best, list );
            }
            else
                res = 0;
        }
        deck_dispose( d );
    }
    return res;
}
/**
 * Update the pointers to start and end of the mum if needed
 * @param a the alignment to update
 * @param list the list of cards
 * @return 1 if it worked else 0
 */
int alignment_update( alignment *a, card *list )
{
    int res = 1;
    if ( !a->stale )
        res = 1;
    else if ( a->best != NULL )
    {
        res = mum_update( a->best, list );
        a->stale = 0;
    }
    else
        res = 0;
    return res;
}
/**
 * Merge the chosen MUM into the pairs array
 * @param a the alignment to merge into the MVD
 * @param left the leftover alignment on the left
 * @param right the leftover alignment on the right
 * @param discards an array of discarded small cards
 * @return 1 if it worked
 */
int alignment_merge( alignment *a, alignment **left, 
    alignment **right, dyn_array *discards )
{
    int res = 1;
    res = alignment_create_lhs( a, left );
    if ( res )
    {
        mum *last = a->best;
        res = mum_split( a->best, a->text, a->version, a->o, discards, a->log );
        if ( res )
        {
            int dist;
            int tlen = alignment_tlen(a);
            if ( mum_transposed(a->best,a->version,tlen,&dist) )
                res = alignment_transpose_merge( a, discards );
            else
                res = alignment_direct_merge( a );
            if ( res )
            {
                while ( mum_next(last) != NULL )
                    last = mum_next(last);
                res = alignment_create_rhs( a, last, right );
                if ( !res )
                    plugin_log_add(a->log,"alignment: failed to create rhs\n");
            }
        }
        else
            plugin_log_add(a->log,"alignment: failed to split match\n");
    }
    else
        plugin_log_add(a->log,"alignment: failed to create lhs\n");
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
 * Get the end of the alignment's new text
 * @param a the alignment
 * @return the offset of the character beyond the last one
 */
int alignment_end( alignment *a )
{
    return a->start+a->len;
}
/**
 * Get this length of our section of the new version text
 * @param a the alignment
 * @return the length of the new unaligned pair
 */
int alignment_len( alignment *a )
{
    return a->len;
}
/**
 * Get the total length of the new version text
 * @param a the alignment
 * @return the length of the underlying text
 */
int alignment_tlen( alignment *a )
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
/**
 * Get the starting position int he new version
 * @param a the alignment in question
 * @return the st_off for this alignment
 */
int alignment_start( alignment *a )
{
    return a->start;
}
/**
 * Convert the new version text of a alignment to a card
 * @param d the decl to convert
 * @param log the log to record errors in
 * @return the alignment's new version text and version as a card
 */
card *alignment_to_card( alignment *d, plugin_log *log )
{
    card *c = NULL;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, d->version );
        UChar *data = calloc( d->len, sizeof(UChar) );
        if ( data != NULL )
        {
            memcpy( data, &d->text[d->start], sizeof(UChar)*d->len );
            pair *p = pair_create_basic( bs, data, d->len );
            if ( p != NULL )
            {
                c = card_create( p, log );
                card_set_text_off( c, d->start );
            }
            free( data );
        }
        bitset_dispose( bs );
    }
    return c;
}