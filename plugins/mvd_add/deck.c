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
#include "deck.h"
#include "hashmap.h"
#include "match.h"
#include "matcher.h"
#include "utils.h"
#include "benchmark.h"

// not used
#define MAX_THREADS 8
#define NUM_SEGMENTS 12
UChar USTR_EMPTY[] = {0};

struct deck_struct 
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
    // next deck in queue
    deck *next;
};
/**
 * Create an deck between a bit of text and the MVD.
 * @param text the text of the whole new version in UTF-16
 * @param start the start offset in text
 * @param tlen the length of text in UChars
 * @param version the ID of the new version
 * @param o the orphanage to store unattached children in
 * @param log the log to record errors in
 */
deck *deck_create( UChar *text, int start, int len, int tlen, 
    int version, orphanage *o, plugin_log *log )
{
    deck *a = calloc( 1, sizeof(deck) );
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
            deck_dispose( a );
            plugin_log_add(log,"deck: failed to create suffix tree\n");
            a = NULL;
        }
    }
    else
        plugin_log_add( log, "deck: failed to allocate object\n");
    return a;
}
/**
 * Dispose of an deck.
 * @param a the deck to dispose
 */
void deck_dispose( deck *a )
{
    if ( a->st != NULL )
        suffixtree_dispose( a->st );
    // text and orphanage belongs to the caller
    free( a );
}
/**
 * Get the textual component of this deck
 * @param a the deck object
 * @param tlen VAR param: its length
 * @return the text in UTF-16 format
 */
UChar *deck_text( deck *a, int *tlen )
{
    *tlen = a->tlen;
    return a->text;
}
/**
 * Get the log for this deck
 * @param a the deck
 * @return a plugin log
 */
plugin_log *deck_log( deck *a )
{
    return a->log;
}
/**
 * Append an deck on the end of another. Keep sorted on decreasing length.
 * @param head the head of the current deck
 * @param next the new one to add at the end
 * @return the new or old head (unchanged)
 */
deck *deck_push( deck *head, deck *next )
{
    deck *prev = NULL;
    deck *temp = head;
    while ( temp != NULL && deck_len(temp) > deck_len(next) )
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
            plugin_log_add(head->log,"deck: error in deck list!\n");
    }*/
    return head;
}
/**
 * Remove from the head of the list
 * @param head the list start
 * @return the new head
 */
deck *deck_pop( deck *head )
{
    return head->next;
}
/**
 * Debug routine
 * @param a the deck to print
 * @param prompt the prefix to the deck
 */
void deck_print( deck *a, const char *prompt )
{
    int end = a->start+a->len-1;
    printf( "%s v=%d %d-%d\n", prompt,
        a->version,a->start,end);
}
/**
 * Merge a transposed MUM into the MVD pairs list. Match is already split.
 * @param mum the mum to merge
 * @param a the deck in question
 */
static void deck_transpose_merge( deck *a, match *mum )
{
    do
    {
        card *temp = match_start_link( mum );
        card *end = match_end_link( mum );
        bitset *mv = match_versions( mum );
        int text_off = match_text_off( mum );
        do
        {
            // make every pair in the match a parent:
            // either already a parent, or an ordinary pair or a child
            card *parent = card_make_parent( temp, a->log );
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
                orphanage_add_child( a->o, child );
                text_off += pair_len(ppair);
            }
            else // none of those
                plugin_log_add(a->log,"deck: failed to create parent\n");
            temp = card_next( temp, mv );
        } while ( temp != end );
        mum = match_next( mum );
    } while ( mum != NULL );
}
/**
 * Merge a MUM directly into the MVD pairs list
 * @param mum the mum to merge (may be a list)
 * @param start_p the first card of the match, starting at offset 0
 * @param end_p the last card of the match, ending at card end
 * @param a the deck to merge within
 */
static void deck_direct_merge( deck *a, match *mum )
{
    do
    {
        card *temp = match_start_link( mum );
        card *end_p = match_end_link( mum );
        bitset *mv = match_versions(mum);
        int text_off = match_text_off(mum);
        // align the middle bit
        int v = deck_version( a );
        do
        {
            pair *p = card_pair(temp);
            bitset *bs = pair_versions(p);
            bitset_set( bs, v );
            card_set_text_off( temp, text_off );
            text_off += pair_len(p);
            if ( temp != end_p )
                temp = card_next( temp, mv );
        } while ( temp != end_p );
        mum = match_next( mum );
    } while ( mum != NULL );
}
/**
 * Split the left-hand-side off from the merged deck
 * @param a the deck whose mum has been computed
 * @param mum the mum in question
 * @param left update with the new left-hand deck if any or NULL
 * @return 1 if it worked
 */
static int deck_create_lhs( deck *a, match *mum, deck **left )
{
    int res = 1;
    int v = deck_version( a );
    if ( match_text_off(mum)>0 )
    {
        int llen = match_text_off(mum)-a->start;
        *left = deck_create( a->text, a->start, llen, a->tlen, v, 
            a->o, a->log );
        if ( *left == NULL )
        {
            plugin_log_add(a->log,"deck: failed to create lhs\n");
            res = 0;
        }
    }
    else
        *left = NULL;
    return res;
}
/**
 * Split off the rhs of a match
 * @param a the deck
 * @param last the last mum in the sequence (if several)
 * @return 1 if it worked else 0
 */
int deck_create_rhs( deck *a, match *last, deck **right )
{
    int res = 1;
    int v = deck_version( a );
    if ( match_text_end(last) < a->tlen )
    {
        int rlen = (a->start+a->len)-match_text_end(last);
        *right = deck_create( a->text,match_text_end(last),rlen,a->tlen,
            v,a->o,a->log);
        if ( *right == NULL )
        {
            plugin_log_add(a->log,"deck: failed to create lhs\n");
            res = 0;
        }
    }
    else // not an error
        *right = NULL;  
    return res;
}
/**
 * Merge the chosen MUM into the pairs array
 * @param a the deck to merge into the MVD
 * @param mum the mum - transpose or direct
 * @param left the leftover deck on the left
 * @param right the leftover deck on the right
 * @return 1 if it worked
 */
static int deck_merge( deck *a, match *mum, deck **left, deck **right )
{
    int res = 1;
    res = deck_create_lhs( a, mum, left );
    if ( res )
    {
        match *last = mum;
        res = match_split( mum, a->text, a->version, a->log );
        if ( res )
        {
            if ( match_transposed(mum,deck_version(a),deck_tlen(a)) )
                deck_transpose_merge( a, mum );
            else
                deck_direct_merge( a, mum );
            while ( match_next(last) != NULL )
                last = match_next(last);
            res = deck_create_rhs( a, last, right );
            if ( !res )
                plugin_log_add(a->log,"deck: failed to create rhs\n");
        }
        else
            plugin_log_add(a->log,"deck: failed to split match\n");
    }
    else
        plugin_log_add(a->log,"deck: failed to create lhs\n");
    return res;
}
/**
 * Align with the whole text excluding those segments already aligned
 * @param head the list of decks
 * @param pairs the list of pairs to be searched
 * @param left VAR param set to the leftover deck on the left
 * @param right VAR param set to the leftover deck on the right
 * @return 1 if it merged correctly, else 0
 */
int deck_align( deck *a, card **cards,  
    deck **left, deck **right, orphanage *o )
{
    int res = 0;
    matcher *m = matcher_create( a, *cards );
    if ( m != NULL )
    {
        // res==1 if there is at least one match
        if ( res = matcher_align(m) )
        {
            match *mum = matcher_get_mum( m );
            // all matches may have freq > 1
            if ( mum != NULL )
            {
                match *temp = mum;
                // debug
                while ( match_next(temp)!=NULL )
                    temp = match_next(temp);
                int end = match_text_end(temp);
                printf("merging %d-%d\n",match_text_off(mum),end);
                // end debug
                res = deck_merge( a, mum, left, right );
            }
            else
                res = 0;
        }
        if ( !res )
        // failed to find a suitable match: add a new pair instead
        {
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bs = bitset_set( bs, a->version );
                if ( bs != NULL )
                {
                    pair *p = pair_create_basic(bs, &a->text[a->start], 
                        a->len);
                    //pair_print( p );
                    card *lp = card_create( p, a->log );
                    card_set_text_off( lp, a->start );
                    card_add_at( cards, lp, a->start, a->version );
                    bitset_dispose( bs );
                    res = 1;
                }
            }
        }
        matcher_dispose( m );
    }
    else
        res = 0;
    return res;
}
/**
 * Get this deck's new version
 * @param a the deck
 * @return its version number for the new version
 */
int deck_version( deck *a )
{
    return a->version;
}
/**
 * Get this length of our section of the new version text
 * @param a the deck
 * @return the length of the new unaligned pair
 */
int deck_len( deck *a )
{
    return a->len;
}
/**
 * Get the total length of the new version text
 * @param a the deck
 * @return the length of the underlying text
 */
int deck_tlen( deck *a )
{
    return a->tlen;
}
/**
 * Get this deck's suffixtree
 * @param a the laignment in question
 * @return a suffixtree made of this deck's text
 */
suffixtree *deck_suffixtree( deck *a )
{
    return a->st;
}
/**
 * Get the starting position int he new version
 * @param a the deck in question
 * @return the st_off for this deck
 */
int deck_start( deck *a )
{
    return a->start;
}