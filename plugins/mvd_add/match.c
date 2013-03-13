#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "bitset.h"
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "match.h"
#define KDIST 2
struct match_struct
{
    // index of first matched/unmatched pair
    int start_p;
    // index of last matched/unmatched pair
    int end_p;
    // index into data of first pair where we started this match
    int start_pos;
    // index into data of last pair of current match/mismatch
    int end_pos;
    // offset of match in suffixtree's underlying string
    int st_off;
    // last matched value of end_p
    int prev_p;
    // last matched value of end_pos
    int prev_pos;
    // length of match
    int len;
    // index of last pair
    int end;
    // number of times this match has been found
    int freq;
    // pairs array - read only
    pair **pairs;
    // suffix tree of new version to match pairs against - read only
    suffixtree *st;
    // cumulative ANDed versions of current matched path
    bitset *bs;
    // next match in this sequence satisfying certain rigid criteria
    match *next;
};
match *match_create( int i, int j, pair **pairs, suffixtree *st, 
    int end, plugin_log *log )
{
    match *mt = calloc( 1, sizeof(match) );
    if ( mt != NULL )
    {
        mt->prev_p = mt->end_p = mt->start_p = i;
        mt->prev_pos = mt->end_pos = mt->start_pos = j;
        mt->pairs = pairs;
        mt->st = st;
        mt->end = end;
    }
    else
        plugin_log_add( log, "match: failed to create match object\n");
    return mt;
}
/**
 * Clone a match object making ready to continue from where it left off
 * @param mt the match to copy
 * @param log the log to report errors to
 * @return a copy of mt or NULL on failure
 */
match *match_clone( match *mt, plugin_log *log )
{
    match *mt2 = calloc( 1, sizeof(match) );
    if ( mt2 != NULL )
    {
        *mt2 = *mt;
        // pick up where our parent left off
        mt2->start_p = mt->end_p;
        mt2->start_pos = mt->end_pos;
        mt2->len = 0;
        mt->end_p = mt->prev_p;
        mt->end_pos = mt->prev_pos; 
    }
    else
        plugin_log_add( log, "match: failed to create match object\n");
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
    free( m );
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
 * Get the next pair in an array
 * @param pairs an array of pairs
 * @param end index of last pair
 * @param pos position of first pair to look at
 * @param bs the bitset of versions to follow
 * @return -1 on failure else the index of the next pair
 */
static int next_pair( pair **pairs, int end, int pos, bitset *bs )
{
    int res = -1;
    int i = pos;
    while ( i<=end && !bitset_intersects(bs,pair_versions(pairs[i])) )
        i++;
    if ( i <= end )
        res = i;
    return res;
}
/**
 * Restart a match by restarting it one character further on
 * @param m the match to restart
 * @return 1 if it worked else 0 (ran out of text)
 */
int match_restart( match *m )
{
    int old_p = m->start_p;
    if ( m->start_pos == pair_len( m->pairs[m->start_p]) )
    {
        do
        {
            m->start_p = next_pair( m->pairs, m->end, m->start_p+1, m->bs );
        }
        while (m->start_p != -1 && pair_len(m->pairs[m->start_p])==0 );
        if ( m->start_p != -1 )
            m->start_pos = 0;
        else
            return 0;
    }
    else
        m->start_pos++;
    if ( old_p != m->start_p )
        bitset_and( m->bs, pair_versions(m->pairs[m->start_p]) );
    m->end_pos = m->start_pos;
    m->end_p = m->start_p;
    m->prev_p = m->end_p;
    m->prev_pos = m->end_pos;
    m->len = 0;
    return 1;
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
    if ( second->start_p == first->end_p )
        pairs_dist = second->start_pos-first->end_pos;
    // 2. compute distance between pairs
    else if ( second->start_p > first->end_p )
    {
        pairs_dist = pair_len(first->pairs[first->end_p])-(first->end_pos+1);
        int i = first->end_p;
        while ( pairs_dist <= KDIST )
        {
            i = next_pair( first->pairs,first->end,i+1, first->bs );
            if ( i != -1 )
            {
                int p_len = pair_len(first->pairs[i]);
                if ( i < second->start_p )
                    pairs_dist += p_len;
                else if ( i == second->start_p )
                    pairs_dist += second->end_pos;
            }
            else
                break;
            i++;
        }
    }
    else
        pairs_dist = INT_MAX;
    // compute distance between matches in the suffixtree
    st_dist = second->st_off - (first->st_off+first->len);
    if ( pairs_dist <= KDIST && st_dist>= 0 && st_dist <= KDIST  )
        res = 1;
    return res;
}
/**
 * Is a match maximal - i.e. does it not extend backwards?
 * @param m the match or string of matches
 * @param text the text of the new version
 * @return 1 if it is maximal else 0
 */
int is_maximal( match *m, UChar *text )
{
    if ( m->st_off == 0 )
        return 1;
    else if ( m->start_pos > 0 )
    {
        UChar *data = pair_data( m->pairs[m->start_p] );
        return text[m->st_off-1] != data[m->start_pos-1];
    }
    else if ( m->start_p == 0 )
    {
        return 1;
    }
    else
    {
        int i = m->start_p-1;
        do
        {
            pair *p = m->pairs[i];
            if ( bitset_intersects( m->bs, pair_versions(p)) )
            {
                if ( pair_len(p) > 0 )
                {
                    UChar *data = pair_data(p);
                    return text[m->st_off-1] != data[pair_len(p)-1];
                }
            }
        }
        while ( i >= 0 );
    }
}
/**
 * Advance the match position
 * @param m the matcher instance
 * @param mt the match object
 * @return 1 if we could advance else 0 if we reached the end
 */
int match_advance( match *m )
{
    int res = 0;
    if ( m->end_p <= m->end )
    {
        m->prev_p = m->end_p;
        m->prev_pos = m->end_pos;
        if ( pair_len(m->pairs[m->end_p])-1==m->end_pos )
        {
            int i;
            for ( i=m->end_p+1;i<=m->end;i++ )
            {
                bitset *bs = pair_versions(m->pairs[i]);
                if ( bitset_intersects(m->bs,bs) )
                {
                    bitset_and( m->bs, bs );
                    if ( pair_len(m->pairs[i])> 0 )
                    {
                        m->end_pos = 0;
                        break;
                    }
                }
            }
            // point to next pair or beyond the end
            m->end_p = i;
        }
        else
            m->end_pos++;
        res = 1;
    }
    return res;
}
/**
 * Complete a single match between the pairs list and the suffixtree
 * @param m the match all ready to go
 * @param text the text of the new version
 * @return 1 if the match was at least 1 char long else 0
 */
int match_single( match *m, UChar *text )
{
    UChar c;
    pos loc;
    int maximal = 0;
    loc.v = suffixtree_root( m->st );
    loc.loc = node_start(loc.v)-1;
    do 
    {
        UChar *data = pair_data(m->pairs[m->end_p]);
        c = data[m->end_pos];
        if ( suffixtree_advance_pos(m->st,&loc,c) )
        {
            if ( !maximal && node_is_leaf(loc.v) )
            {
                m->st_off = node_start(loc.v)-m->len;
                if ( !is_maximal(m,text) )
                    break;
                else
                    maximal = 1;
            }
            if ( match_advance(m) )
                m->len++;
            else
                break;
        }
        else
            break;
    }
    while ( 1 );
    return maximal;
}
int match_start_index( match *m )
{
    return m->start_p;
}
int match_prev_pos( match *m )
{
    return m->prev_pos;
}

int match_inc_end_pos( match *m )
{
    return ++m->end_pos;
}
suffixtree *match_suffixtree( match *m )
{
    return m->st;
}
int match_len( match *m )
{
    return m->len;
}
int match_total_len( match *m )
{
    int len = m->len;
    while ( m->next != NULL )
    {
        m = m->next;
        len += m->len;
    }
    return len;
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
    int len1 = one->len;
    int len2 = two->len;
    while ( one->next != NULL )
    {
        one = one->next;
        len1 += one->len;
    }
    while ( two->next != NULL )
    {
        two = two->next;
        len2 += two->len;
    }
    if ( len1>len2 )
        return 1;
    else if ( len2>len1 )
        return -1;
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
        print_utf8(&text[temp->st_off],temp->len );
        if ( temp->next != NULL )
        {
            int end = temp->st_off+temp->len;
            int diff = temp->next->st_off-end;
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