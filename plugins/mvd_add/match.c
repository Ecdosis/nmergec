#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "hashmap.h"
#include "dyn_array.h"
#include "linkpair.h"
#include "match.h"
#include "match_state.h"
#define KDIST 2

struct match_struct
{
    // index of first matched/unmatched pair
    linkpair *start_p;
    // index of last matched/unmatched pair
    linkpair *end_p;
    // index into data of first pair where we started this match
    int start_pos;
    // inclusive index into data of last pair of current match/mismatch
    int end_pos;
    // offset of match in the local suffixtree's string
    int st_off;
    // last matched value of end_p
    linkpair *prev_p;
    // last matched value of end_pos
    int prev_pos;
    // length of match
    int len;
    // number of times this match has been found
    int freq;
    // pairs array - read only
    linkpair *pairs;
    // suffix tree of new version to match pairs against - read only
    suffixtree *st;
    // cumulative ANDed versions of current matched path
    bitset *bs;
    // next match in this sequence satisfying certain rigid criteria
    match *next;
    // queue of match branches not yet followed
    match_state *queue;
    // location in suffix tree to which match has been verified
    pos loc;
};
/**
 * Create a match
 * @param start the linkpair to start matching from
 * @param j the start position within start to start from
 * @param pairs the overall linked list of pairs
 * @param st the suffix tree of the section of text we are matching against
 * @param log the log to write errors to
 * @return  a match object
 */
match *match_create( linkpair *start, int j, linkpair *pairs, suffixtree *st, 
    plugin_log *log )
{
    match *mt = calloc( 1, sizeof(match) );
    if ( mt != NULL )
    {
        mt->prev_p = mt->end_p = mt->start_p = start;
        mt->prev_pos = mt->end_pos = mt->start_pos = j;
        mt->pairs = pairs;
        mt->st = st;
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
        mt2->start_p = mt->start_p;
        mt2->end_p = mt->end_p;
        mt2->start_pos = mt->start_pos;
        mt2->end_pos = mt->end_pos;
        mt2->st_off = mt->st_off;
        mt2->prev_p = mt->prev_p;
        mt2->prev_pos = mt->prev_pos;
        mt2->len = mt->len;
        mt2->freq = mt->freq;
        mt2->pairs = mt->pairs;
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
    if ( mt->end_p != NULL )
    {
        mt2 = match_copy( mt, log );
        if ( mt2 != NULL )
        {
            // pick up where our parent left off
            mt2->start_p = mt->end_p;
            mt2->start_pos = mt->end_pos;
            mt2->len = 0;
            mt->end_p = mt->prev_p;
            mt->end_pos = mt->prev_pos; 
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
        m->start_p = match_state_start_p(ms);
        m->end_p = match_state_end_p(ms);
        match_state_loc( ms, &m->loc );
        m->start_p = match_state_start_p( ms );
        m->end_p = match_state_end_p( ms );
        m->start_pos = match_state_start_pos( ms );
        m->end_pos = match_state_end_pos( ms );
        m->st_off = match_state_st_off( ms );
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
 * Restart a match by restarting it one character further on
 * @param m the match to restart
 * @return 1 if it worked else 0 (ran out of text)
 */
int match_restart( match *m )
{
    linkpair *old_p = m->start_p;
    pair *sp = linkpair_pair(m->start_p);
    if ( m->start_pos == pair_len(sp) )
    {
        do
        {
            linkpair *lpr = linkpair_right(m->start_p);
            if ( lpr != NULL )
            {
                m->start_p = linkpair_next( lpr, m->bs );
                if ( m->start_p != NULL )
                    sp = linkpair_pair(m->start_p);
            }
            else
                m->start_p = NULL;
        }
        while (m->start_p != NULL && pair_len(sp)==0 );
        if ( m->start_p != NULL )
            m->start_pos = 0;
        else
            return 0;
    }
    else
        m->start_pos++;
    if ( old_p != m->start_p )
        bitset_and( m->bs, pair_versions(sp) );
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
        pairs_dist = (second->start_pos-first->end_pos)-1;
    // 2. compute distance between pairs
    else if ( second->start_p != first->end_p )
    {
        pairs_dist = pair_len(linkpair_pair(first->end_p))-(first->end_pos+1);
        linkpair *lp = linkpair_right(first->end_p);
        while ( pairs_dist <= KDIST )
        {
            linkpair *next = linkpair_next( lp, first->bs );
            if ( next != NULL )
            {
                int p_len = pair_len(linkpair_pair(first->start_p));
                if ( lp != second->start_p )
                    pairs_dist += p_len;
                else if ( lp == second->start_p )
                    pairs_dist += second->start_pos;
            }
            else
                break;
            lp = linkpair_right( lp );
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
        UChar *data = pair_data( linkpair_pair(m->start_p) );
        return text[m->st_off-1] != data[m->start_pos-1];
    }
    else if ( m->start_p == 0 )
    {
        return 1;
    }
    else
    {
        linkpair *lp = linkpair_left(m->start_p);
        while ( lp != NULL )
        {
            pair *p = linkpair_pair(lp);
            if ( bitset_intersects( m->bs, pair_versions(p)) )
            {
                if ( pair_len(p) > 0 )
                {
                    UChar *data = pair_data(p);
                    return text[m->st_off-1] != data[pair_len(p)-1];
                }
            }
            lp = linkpair_left(lp);
        }
        return 0;
    }
}
/**
 * Advance the match position
 * @param m the match object
 * @param loc the location in the suffix tree matched to so far
 * @param log the log to record errors in 
 * @return 1 if we could advance else 0 if we reached the end
 */
int match_advance( match *m, pos *loc, plugin_log *log )
{
    int res = 1;
    if ( m->end_p != NULL )
    {
        linkpair *lp = m->end_p;
        m->prev_p = m->end_p;
        m->prev_pos = m->end_pos;
        if ( pair_len(linkpair_pair(lp))-1==m->end_pos )
        {
            lp = linkpair_right( lp );
            if ( lp == NULL )
            {
                m->end_pos = pair_len( linkpair_pair(m->end_p) );
                res = 0;
            }
            else while ( lp != NULL )
            {
                pair *p = linkpair_pair(lp);
                bitset *bs = pair_versions( p );
                if ( pair_len(p)==0 || !bitset_intersects(m->bs,bs) )
                    lp = linkpair_right(lp);
                else
                {
                    if ( !bitset_equals(m->bs,bs) )
                    {
                        bitset *bs2 = bitset_clone( m->bs );
                        bitset_and_not( bs2, bs );
                        match_state *ms = match_state_create(
                            m->start_p, m->end_p, 
                            m->start_pos, m->end_pos, m->st_off, 
                            m->len, bs2, loc, log );
                        if ( m->queue == NULL )
                            m->queue = ms;
                        else
                            match_state_push( m->queue, ms );
                        // restrict this match to ANDed versions
                        bitset_and( m->bs, bs );
                    }
                    m->end_pos = 0;
                    m->end_p = lp;
                    break;
                }
            }
        }
        else
            m->end_pos++;
    }
    return res;
}
/**
 * Can we extend this match or are we at the end of the linkpair list?
 * @param m the match to extend
 * @return 1 if it can be extended theoretically, else 0
 */
static int match_extendible( match *m )
{
    return m->end_pos!=pair_len(linkpair_pair(m->end_p));
}
/**
 * Extend a match as far as you can
 * @param mt the match to extend
 * @param text the UTF-16 text of the new version
 * @param log the log to record errors in
 * @return the extended/unextended match
 */
match *match_extend( match *mt, UChar *text, plugin_log *log )
{
    match *last = mt;
    match *first = mt;
    do
    {
        match *mt2 = (match_extendible(mt))?match_clone(mt,log):NULL;
        if ( mt2 != NULL )
        {
            int distance = 1;
            do  
            {
                if ( match_single(mt2,text,log) && match_follows(last,mt2) )
                {// success
                    match_append(last,mt2);
                    mt = last = mt2;
                    break;
                }
                else 
                {// failure: reset and move to next position
                    if ( distance < KDIST )
                    {
                        if ( match_restart(mt2) )
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
 * @param log the log to save errors in
 * @return 1 if the match was at least 1 char long else 0
 */
int match_single( match *m, UChar *text, plugin_log *log )
{
    UChar c;
    int maximal = 0;
    pos *loc = &m->loc;
    loc->v = suffixtree_root( m->st );
    loc->loc = node_start(loc->v)-1;
    do 
    {
        UChar *data = pair_data(linkpair_pair(m->end_p));
        c = data[m->end_pos];
        if ( suffixtree_advance_pos(m->st,loc,c) )
        {
            if ( !maximal && node_is_leaf(loc->v) )
            {
                m->st_off = node_start(loc->v)-m->len;
                if ( !is_maximal(m,text) )
                    break;
                else
                    maximal = 1;
            }
            if ( match_advance(m,loc,log) )
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
linkpair *match_start_link( match *m )
{
    return m->start_p;
}
int match_start_pos( match *m )
{
    return m->start_pos;
}
linkpair *match_end_link( match *m )
{
    return m->end_p;
}
int match_end_pos( match *m )
{
    return m->end_pos;
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
int match_st_off( match *m )
{
    return m->st_off;
}
int match_st_end( match *m )
{
    return m->st_off+m->len;
}
/** 
 * Create a linkpair that contains a short sequence of the new version
 * @param m1 the first match before m2
 * @param m2 the second after m1
 * @param v the id of the new version
 */
static linkpair *linkpair_between_matches( match *m1, match *m2, 
    UChar *text, int v, plugin_log *log )
{
    linkpair *between = NULL;
    int st_len = match_st_off(m2)-match_st_end(m1);
    UChar *fragment = calloc( st_len+1, sizeof(UChar) );
    if ( fragment != NULL )
    {
        memcpy( fragment, &text[match_st_end(m1)], st_len*sizeof(UChar) );
        bitset *bs = bitset_create();
        if ( bs != NULL )
        {
            bitset_set( bs, v );
            pair *frag = pair_create_basic( bs, fragment, st_len );
            if ( frag != NULL )
                between = linkpair_create( frag, log );
        }
        free( fragment );
    }
    return between;
}
/**
 * Split the underlying matches of the MVD. NB start_pos/end_pos are inclusive
 * @param m the match
 * @param text the text the match is aligned with
 * @param v the number of the new version
 * @param log the log to write errors to
 */
void match_split( match *m, UChar *text, int v, plugin_log *log )
{
    match *temp = m;
    int i;
    dyn_array *matches = dyn_array_create(5);
    if ( matches != NULL )
    {
        dyn_array_add( matches, temp );
        while ( temp->next != NULL )
        {
            temp = temp->next;
            dyn_array_add( matches, temp );
        }
        for ( i=dyn_array_size(matches)-1;i>=0;i-- )
        {
            temp = dyn_array_get( matches, i );
            if ( temp->end_pos < pair_len(linkpair_pair(temp->end_p))-1
                && temp->end_pos > 0 )
                linkpair_split( temp->end_p, temp->end_pos );
            if ( temp->start_pos > 0 
                && temp->start_pos < pair_len(linkpair_pair(temp->start_p))-1 )
            {
                linkpair *old_start_p = temp->start_p;
                linkpair_split( temp->start_p, temp->start_pos );
                temp->start_p = linkpair_right( temp->start_p );
                if ( old_start_p==temp->end_p )
                    temp->end_p = temp->start_p;
            }
            temp->start_pos = 0;
            temp->end_pos = pair_len(linkpair_pair(temp->end_p))-1;
            if ( linkpair_right(temp->end_p) == NULL )
                temp->end_pos++;
            if ( i < dyn_array_size(matches)-1 )
            {
                // add a short linking segment
                linkpair *betw = linkpair_between_matches(temp,
                    temp->next, text, v, log );
                if ( betw != NULL )
                {
                    if ( linkpair_node_to_right(temp->end_p) )
                    {
                        // temporarily invalidates the pairs-list
                        // but once we add v to temp->end_p
                        // betw will form a node with temp->end_p 
                        // and betw->right will also attach to it
                        linkpair_add_at_node( temp->end_p, betw );
                    }
                    else
                        linkpair_add_after( temp->end_p, betw );
                }
                else
                    plugin_log_add(log,"match: failed to join matches\n");
            }
        }
        dyn_array_dispose( matches );
    }
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
 * Work out if this match is transposed
 * 1. find left and right closet direct alignments or the end 
 * 2. find the st_offset of each such alignment
 * 3. if our st_off is between these two, then it is direct, 
 * else it is transposed
 * @param m the match to test
 * @param new_version the ID of the new version
 * @param tlen length of new version text
 * @return 1 if it is transposed else 0
 */
int match_transposed( match *m, int new_version, int tlen )
{
    int st_left = 0;
    int st_right = tlen;
    // 1. find left direct align
    linkpair *left = linkpair_left(m->start_p);
    while ( left != NULL )
    {
        pair *p = linkpair_pair( left );
        bitset *bs = pair_versions(p);
        if ( bitset_next_set_bit(bs,new_version)==new_version 
            && bitset_cardinality(bs)>1 )
        {
            st_left = pair_len(p)+linkpair_st_off(left);
            break;
        }
        left = linkpair_left(left);
    } 
    linkpair *right = linkpair_right(m->end_p);
    while ( right != NULL )
    {
        pair *p = linkpair_pair( right );
        bitset *bs = pair_versions(p);
        if ( bitset_next_set_bit(bs,new_version)==new_version 
            && bitset_cardinality(bs)>1 )
        {
            st_right = linkpair_st_off(right);
            break;
        }
        right = linkpair_right(right);
    }
    if ( m->st_off > st_left && m->st_off+m->len < st_right )
        return 0;
    else
        return 1;
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