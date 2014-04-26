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
#include <stdio.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "state.h"
#include "aatree.h"
#include "dyn_array.h"
#include "card.h"
#include "orphanage.h"
#include "location.h"
#include "match.h"
#include "alignment.h"
#include "deck.h"
#include "hashmap.h"
#include "utils.h"
#include "mum.h"

#define PQUEUE_LIMIT 50

/**
 * A deck is something that looks for matches by comparing the 
 * list of pairs (cards) with the suffix tree, which itself represents
 * the new version. After matching is ended the deck returns the 
 * best MUM.
 */
struct deck_struct
{
    suffixtree *st;
    card *cards;
    UChar *text;
    int tlen;
    aatree *pq;
    int version;
    int st_off;
    plugin_log *log;
};
/**
 * Create a deck
 * @param a the alignment object
 * @param cards the list of pairs from the MVD turned into cards
 * @return a deck object ready to go
 */
deck *deck_create( alignment *a, card *cards )
{
    deck *d = calloc( 1, sizeof(deck) );
    if ( d != NULL )
    {
        d->log = alignment_log( a );
        d->cards = cards;
        d->text = alignment_text( a, &d->tlen );
        d->version = alignment_version(a);
        d->st = alignment_suffixtree( a );
        d->st_off = alignment_start(a);
        d->pq = aatree_create( match_compare, PQUEUE_LIMIT );
        if ( d->pq == NULL )
        {
            deck_dispose( d );
            d = NULL;
        }
    }
    else
        plugin_log_add(d->log, "deck: failed to allocate object\n");
    return d;
}
/**
 * Dispose of a deck object
 * @param m the deck in question
 */
void deck_dispose( deck *d )
{
    if ( d->pq != NULL )
        aatree_dispose( d->pq, (aatree_dispose_func)match_dispose );
    free( d );
}
/**
 * Perform an alignment recursively
 * @param m the deck to do the alignment within
 * @return 1 if it worked, else 0 on error
 */
int deck_align( deck *d )
{
    card *c = d->cards;
    while ( c != NULL )
    {
        pair *p = card_pair( c );
        bitset *bs = pair_versions(p);
        // ignore pairs already aligned with the new version
        if ( bitset_next_set_bit(bs,d->version)!=d->version )
        {
            int j,length = pair_len( p );
            for ( j=0;j<length;j++ )
            {
                // start a match at each character position
                match *mt = match_create( c, j, d->cards, d->st, 
                    d->st_off, d->log );
                if ( mt != NULL )
                {
                    match_set_versions( mt, bitset_clone(bs) );
                    while ( mt != NULL )
                    {
                        match *queued = NULL;
                        match *existing = NULL;
                        if ( match_single(mt,d->text,d->version,d->log) )
                        {
                            queued = match_extend( mt, d->text, d->version, d->log );
                            existing = aatree_add( d->pq, queued );
                            if ( existing != NULL )
                                match_inc_freq( existing );
                            // if we added it to the queue, freeze its state
                            if ( existing == queued )
                                mt = match_copy( mt, d->log );
                        }    
                        // does the match have any extensions?
                        if ( mt != NULL && !match_pop(mt) )
                        {
                            match_dispose( mt );
                            mt = NULL;
                        }
                    }
                }
                else
                    break;
            }
        }
        c = card_right( c );
    }
    return !aatree_empty(d->pq);
}
/**
 * Check if a MUM is transposed and passes the transpose criteria
 * @param mt the deck
 * @param mum the mum to test
 * @return 1 if the MUM is worth aligning else 0
 */
int deck_mum_ok( deck *d, match *mt )
{
    int dist;
    if ( mum_transposed((mum*)mt,d->version,d->tlen,&dist) )
       return match_within_threshold(dist,match_total_len(mt));
    else // ALL direct alignments are OK
        return 1;
}
/**
 * Get the longest maximal *unique* match (MUM)
 * @param m the deck in question
 * @return a match, which may be a chain of matches
 */
match *deck_get_mum( deck *d )
{
    match *found = NULL;
    while ( !aatree_empty(d->pq) )
    {
        match *mt = aatree_max( d->pq );
        //match *mt = aatree_min( d->pq );
        if ( match_freq(mt) == 1 && deck_mum_ok(d,mt) )
        {
            found = mt;
            break;
        }
        else if ( !aatree_delete(d->pq,mt) )
            break;
    }
    return found;
}