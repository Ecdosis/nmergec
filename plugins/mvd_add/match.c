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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "hashmap.h"
#include "dyn_array.h"
#include "card.h"
#include "orphanage.h"
#include "location.h"
#include "match.h"
#include "match_state.h"
#define KDIST 2
#define PHI 1.61803399
/*
 * Match represents a sequence of matching charactes in teh pairs list 
 * and in the new text version.
 */
struct match_struct
{
    MATCH_BASE;
    // last matched position in graph
    location prev;
    // offset of our new version segment in the overall text
    int st_off;
    // number of times this match has been found
    int freq;
    // cards list - read only
    card *cards;
    // suffix tree of new version to match pairs against - read only
    suffixtree *st;
    // next match in this sequence satisfying certain rigid criteria
    match *next;
    // queue of match branches not yet followed
    match_state *queue;
    // location in suffix tree to which match has been verified
    pos loc;
};
/**
 * Create a match
 * @param start the card to start matching from
 * @param pos the start position within start to start from
 * @param cards the overall linked list of pairs
 * @param st the suffix tree of the section of text we are matching against
 * @param st_off starting point of the match in the new version
 * @param log the log to write errors to
 * @return  a match object
 */
match *match_create( card *start, int pos, card *cards, suffixtree *st, 
    int st_off, plugin_log *log )
{
    match *mt = calloc( 1, sizeof(match) );
    if ( mt != NULL )
    {
        mt->start.current = start;
        mt->prev.current = start;
        mt->end.current = start;
        mt->prev.pos = pos;
        mt->start.pos = pos;
        mt->end.pos = pos;
        mt->cards = cards;
        mt->st = st;
        mt->st_off = st_off;
    }
    else
        plugin_log_add( log, "match: failed to create match object\n");
    return mt;
}
/**
 * Make an exact deep copy of a match
 * @param mt the match to copy
 * @param log the log to report errors to
 * @return a copy of mt or NULL on failure
 */
match *match_copy( match *mt, plugin_log *log )
{
    match *mt2 = calloc( 1, sizeof(match) );
    if ( mt2 != NULL )
    {
        mt2->start = mt->start;
        mt2->end = mt->end;
        mt2->prev = mt->prev;
        mt2->st_off = mt->st_off;
        mt2->text_off = 0;
        mt2->len = mt->len;
        mt2->freq = mt->freq;
        mt2->cards = mt->cards;
        mt2->st = mt->st;
        mt2->bs = bitset_clone(mt->bs);
        if ( mt->next != NULL )
            mt2->next = match_copy( mt->next, log );
        if ( mt->queue != NULL )
            mt2->queue = match_state_copy(mt->queue,log);
        mt2->loc = mt->loc;
    }
    else
        plugin_log_add( log, "match: failed to duplicate match object\n");
    return mt2;
}
/**
 * Clone a match object making ready to continue from where it left off
 * @param mt the match to copy
 * @param log the log to report errors to
 * @return a copy of mt or NULL on failure
 */
match *match_clone( match *mt, plugin_log *log )
{
    match *mt2 = NULL;
    if ( mt->end.current != NULL )
    {
        mt2 = match_copy( mt, log );
        if ( mt2 != NULL )
        {
            // pick up where our parent left off
            mt2->start = mt->end;
            mt2->len = 0;
            mt->end = mt->prev; 
        }
    }
    return mt2;
}
/**
 * Dispose of a match object
 * @param m the match in question
 */
void match_dispose( match *m )
{
    if ( m->bs != NULL )
        bitset_dispose( m->bs );
    if ( m->next != NULL )
        match_dispose( m->next );
    if ( m->queue != NULL )
    {
        match_state *ms = m->queue;
        while ( ms != NULL )
        {
            match_state *next = match_state_next( ms );
            match_state_dispose( ms );
            ms = next;
        }
    }
    free( m );
}
/**
 * Pop a previous match state back into the match
 * @param m the match state
 * @return 1 if the pop was possible else 0
 */
int match_pop( match *m )
{
    if ( m->queue == NULL )
        return 0;
    else
    {
        match_state *ms = m->queue;
        m->queue = match_state_next( ms );
        match_state_dispose( ms );
        m->start = match_state_start(ms);
        m->end = match_state_end(ms);
        match_state_loc( ms, &m->loc );
        m->text_off = match_state_text_off( ms );
        m->len = match_state_len( ms );
        m->bs = match_state_bs( ms );
        return 1;
    }
}
/**
 * Add a match onto the end of this one
 * @param m1 the first match, us
 * @param m2 the new match
 */
void match_append( match *m1, match *m2 )
{
    match *temp = m1;
    while ( temp->next != NULL )
        temp = temp->next;
    temp->next = m2;
}
/**
 * Does one match follow another within a certain edit distance (KDIST)
 * @param first the first match
 * @param second the second, which should follow it closely
 * @return 1 if they do follow else 0
 */
int match_follows( match *first, match *second )
{
    int res = 0;
    int pairs_dist = 0;
    int st_dist = 0;
    // compute proximity in pairs list 
    // 1. simplest case: same pair
    if ( second->start.current == first->end.current )
        pairs_dist = (second->start.pos-first->end.pos)-1;
    // 2. compute distance between pairs
    else 
    {
        pairs_dist = pair_len(card_pair(first->end.current))-(first->end.pos+1);
        card *lp = first->end.current;
        while ( pairs_dist <= KDIST )
        {
            card *next = card_next( lp, first->bs );
            if ( next != NULL )
            {
                int p_len = pair_len(card_pair(first->start.current));
                if ( lp != second->start.current )
                    pairs_dist += p_len;
                else 
                    pairs_dist += second->start.pos;
            }
            else
                break;
        }
    }
    // compute distance between matches in the suffixtree
    st_dist = second->text_off - (first->text_off+first->len);
    if ( pairs_dist <= KDIST && st_dist>= 0 && st_dist <= KDIST  )
        res = 1;
    return res;
}
/**
 * Is a match maximal - i.e. does it NOT extend backwards?
 * @param m the match or string of matches
 * @param text the text of the new version
 * @return 1 if it is maximal else 0
 */
int is_maximal( match *m, UChar *text )
{
    if ( m->text_off == m->st_off )
        return 1;
    else if ( card_prev(m->start.current,m->bs)==NULL )
    {
        return 1;
    }
    else if ( m->start.pos > 0 )
    {
        UChar *data = pair_data( card_pair(m->start.current) );
        return text[m->text_off-1] != data[m->start.pos-1];
    }
    else
    {
        card *lp = card_prev(m->start.current,m->bs);
        while ( lp != NULL )
        {
            pair *p = card_pair(lp);
            if ( pair_len(p) > 0 )
            {
                UChar *data = pair_data(p);
                return text[m->text_off-1] != data[pair_len(p)-1];
            }
            lp = card_prev(lp,m->bs);
        }
        return 0;
    }
}
/**
 * Restart a match by restarting it one character further on
 * @param m the match to restart
 * @param log the log to record errors in
 * @return 1 if it worked else 0 (ran out of text)
 */
int match_restart( match *m, plugin_log *log )
{
    card *old = m->start.current;
    pair *sp = card_pair(m->start.current);
    if ( m->start.pos == pair_len(sp) )
    {
        card *c = card_next_nonempty(m->start.current,m->bs);
        if ( c != NULL )
        {
            bitset *overlap = card_overlap(c,m->bs);
            if ( overlap != NULL )
            {
                pos loc;
                location pos;
                memset( &pos, 0, sizeof(location) );
                pos.current = c;
                pos.pos = 0;
                loc.loc = 0;
                loc.v = suffixtree_root(m->st);
                match_state *ms = match_state_create(
                    &pos, &pos, m->text_off, 
                    m->len, overlap, &loc, log );
                if ( m->queue == NULL )
                    m->queue = ms;
                else
                    match_state_push( m->queue, ms );
                // restrict the current match to ANDed versions
                bitset_and( m->bs, pair_versions(card_pair(c)) );
            }
            m->start.pos = 0;
            m->start.current = c;
        }
        else
            return 0;
    }
    else
        m->start.pos++;
    if ( old != m->start.current )
        bitset_and( m->bs, pair_versions(card_pair(m->start.current)) );
    m->end = m->start;
    m->prev = m->end;
    m->len = 0;
    return 1;
}
/**
 * Advance the match position - we have already matched the character
 * @param m the match object
 * @param loc the location in the suffix tree matched to so far   
 * @param v the version of the new text. stop when you see this             }
 * @param log the log to record errors in 
 * @return 1 if we could advance else 0 if we reached the end
 */
int match_advance( match *m, pos *loc, int v, plugin_log *log )
{
    int res = 0;
    if ( m->end.current != NULL )
    {
        card *c = m->end.current;
        m->prev = m->end;
        if ( pair_len(card_pair(c))-1==m->end.pos )
        {
            c = card_next_nonempty( c, m->bs );
            if ( c != NULL )
            {
                pair *p = card_pair(c);
                bitset *bs = pair_versions( p );
                if ( bitset_next_set_bit(bs,v)==v )
                    c = NULL;
                else
                {
                    bitset *overlap = card_overlap(c,m->bs);
                    if ( overlap != NULL )
                    {
                        match_state *ms = match_state_create(
                            &m->start, &m->end, m->text_off, 
                            m->len, overlap, loc, log );
                        if ( m->queue == NULL )
                            m->queue = ms;
                        else
                            match_state_push( m->queue, ms );
                        // restrict the current match to ANDed versions
                        bitset_and( m->bs, pair_versions(card_pair(c)) );
                    }
                    m->end.pos = 0;
                    m->end.current = c;
                    res = 1;
                }
            }
            if ( c == NULL )
                m->end.pos = pair_len( card_pair(m->end.current) );
        }
        else
        {
            m->end.pos++;
            res = 1;
        }
    }
    return res;
}
/**
 * Can we extend this match or are we at the end of the card list?
 * @param m the match to extend
 * @return 1 if it can be extended theoretically, else 0
 */
static int match_extendible( match *m )
{
    return m->end.pos!=pair_len(card_pair(m->end.current));
}
/**
 * Extend a match as far as you can
 * @param mt the match to extend
 * @param text the entire UTF-16 text of the new version
 * @param v the new version
 * @param log the log to record errors in
 * @return the extended/unextended match
 */
match *match_extend( match *mt, UChar *text, int v, plugin_log *log )
{
    match *last = mt;
    match *first = mt;
    do
    {
        match *mt2 = (match_extendible(mt))?match_clone(mt,log):NULL;
        if ( mt2 != NULL )
        {
            int distance = 1;
            match_restart( mt2, log );
            do  
            {
                if ( match_single(mt2,text,v,log) && match_follows(last,mt2) )
                {// success
                    match_append(last,mt2);
                    mt = last = mt2;
                    break;
                }
                else 
                {// failure: reset and move to next position
                    if ( distance < KDIST )
                    {
                        if ( match_restart(mt2,log) )
                        {
                            distance++;
                        }
                        else
                        {
                            mt = NULL;
                            break;
                        }
                    }
                    else    // give up
                    {
                        match_dispose( mt2 );
                        last = NULL;
                        break;
                    }
                }
            }
            while ( distance <= KDIST );
        }
        else
            mt = NULL;
    } while ( last == mt );
    return first;
}
/**
 * Complete a single match between the pairs list and the suffixtree
 * @param m the match all ready to go
 * @param text the text of the new version
 * @param v the new version id
 * @param log the log to save errors in
 * @return 1 if the match was at least 1 char long else 0
 */
int match_single( match *m, UChar *text, int v, plugin_log *log )
{
    UChar c;
    int maximal = 0;
    pos *loc = &m->loc;
    loc->v = suffixtree_root( m->st );
    loc->loc = node_start(loc->v)-1;
    do 
    {
        UChar *data = pair_data(card_pair(m->end.current));
        c = data[m->end.pos];
        if ( suffixtree_advance_pos(m->st,loc,c) )
        {
            if ( !maximal && node_is_leaf(loc->v) )
            {
                // m->st_off
                m->text_off = m->st_off + node_start(loc->v)-m->len;
                if ( !is_maximal(m,text) )
                    break;
                else
                    maximal = 1;
            }
            // we are already matched, so increase length
            m->len++;
            if ( !match_advance(m,loc,v,log) )
                break;
        }
        else
            break;
    }
    while ( 1 );
    return maximal;
}
location match_start( match *m )
{
    return m->start;
}
location match_end( match *m )
{
    return m->end;
}
int match_prev_pos( match *m )
{
    return m->prev.pos;
}
int match_inc_end_pos( match *m )
{
    return ++m->end.pos;
}
suffixtree *match_suffixtree( match *m )
{
    return m->st;
}
int match_len( match *m )
{
    return m->len;
}
/**
 * Get the total length of a possibly extended match
 * @param m the matchi n question or NULL
 * @return its overall length including extensions
 */
int match_total_len( match *m )
{
    if ( m == NULL )
        return 0;
    else
    {
        int len = m->len;
        while ( m->next != NULL )
        {
            m = m->next;
            len += m->len;
        }
        return len;
    }
}
void match_set_versions( match *m, bitset *bs )
{
    m->bs = bs;
}
/**
 * Compare two matches based on their length
 * @param a the first match
 * @param b the second match
 * @return 1 if a was greater else if b greater -1 else 0 if equal
 */
int match_compare( void *a, void *b )
{
    match *one = (match*)a;
    match *two = (match*)b;
    int len1 = match_total_len(one);
    int len2 = match_total_len(two);
    if ( len1>len2 )
        return 1;
    else if ( len2>len1 )
        return -1;
    else if ( one != two )
    {
        if ( one->text_off > two->text_off )
            return 1;
        else if ( two->text_off > one->text_off )
            return -1;
        else
            return 0;
    }
    else
        return 0;
}
/**
 * Increment this match's frequency
 * @param m the match in question
 */
void match_inc_freq( match *m )
{
    m->freq++;
}
/**
 * Get the match's frequency
 * @param m the match in question
 * @return the number of times it was found
 */
int match_freq( match *m )
{
    return m->freq;
}
/**
 * Get the versions common along the match path
 * @param m the match in question
 * @return the set of versions
 */
bitset *match_versions( match *m )
{
    return m->bs;
}
/**
 * Get the offset in the new version 
 * @param m the match in question
 * @return offset into m->text where the match starts
 */
int match_text_off( match *m )
{
    return m->text_off;
}
int match_text_end( match *m )
{
    return m->text_off+m->len;
}
/**
 * Get the next match in case it has been extended
 * @return the next match or NULL
 */
match *match_next( match *m )
{
    return m->next;
}
/**
 * Is a transposition too far away or within range
 * @param distance between the edges of the transposition and child in chars
 * @param length length of the transposition
 */
int match_within_threshold( int distance, int length )
{
    double power = pow( (double)length, PHI );
    return power>(double)distance;
}
static char *print_utf8( UChar *ustr, int src_len )
{
    int dst_len = measure_to_encoding( ustr, src_len, "utf-8" );
    char *buf = malloc( dst_len+1 );
    if ( buf != NULL )
    {
        int n_bytes = convert_to_encoding( ustr, src_len, buf, 
            dst_len+1, "utf-8" );
        if ( n_bytes == dst_len )
        {
            buf[dst_len] = 0;
            printf("%s",buf);
        }
        free( buf );
    }
}
/**
 * Print the match to the console
 * @param m the match in question
 * @param text the text of the new version to simplify printing
 */
void match_print( match *m, UChar *text )
{
    match *temp = m;
    while ( temp != NULL )
    {
        print_utf8(&text[temp->text_off],temp->len );
        if ( temp->next != NULL )
        {
            int end = temp->text_off+temp->len;
            int diff = temp->next->text_off-end;
            printf("[");
            if ( diff != 0 )
                print_utf8(&text[end],diff );
            else
                printf("-");
            printf("]");
        }
        temp = temp->next;
    }
    printf("\n");
}