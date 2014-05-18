#include <stdlib.h>
#include <unicode/uchar.h>
#include <string.h>
#include <stdio.h>
#include "plugin_log.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "card.h"
#include "location.h"
#include "bitset.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "match.h"
#include "orphanage.h"
#include "mum.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif


#define MIN(a,b)(a>b)?b:a

/** Manage best match (MaximalUniqueMatch)*/
struct mum_struct
{
    MATCH_BASE;
    // next mum in this sequence satisfying certain rigid criteria
    mum *next;
};

/**
 * Create a mum from a match
 * @param mt the list of matches to copy
 * @param v the new version
 * @param log the log to record errors in
 * @return a finished mum
 */
mum *mum_create( match *mt, int v, plugin_log *log )
{
    mum *m = calloc( 1, sizeof(mum) );
    if ( m != NULL )
    {
        m->start = match_start(mt);
        m->end = match_end(mt);
        m->text_off = match_text_off(mt);
        m->len = match_len(mt);
        m->bs = bitset_clone( match_versions(mt) );
/*
        pair *p1 = card_pair(m->start.current);
        pair *p2 = card_pair(m->end.current);
        if ( bitset_next_set_bit(pair_versions(p1),v)==v
            || bitset_next_set_bit(pair_versions(p2),v)==v )
            printf("error!\n");   
*/
        if ( match_next(mt) != NULL )
            m->next = mum_create( match_next(mt), v, log );
    }
    else
        plugin_log_add(log, "mum: failed to create object");
    return m;
}
/**
 * Dispose of this mum
 * @param m the mum in question
 */
void mum_dispose( mum *m )
{
    if ( m->bs != NULL )
        bitset_dispose( m->bs );
    if ( m->next != NULL )
        mum_dispose( m->next );
    free( m );
}
/**
 * Get the length of a mum
 * @param m the mum in question
 * @return the length of the mum in characters
 */
int mum_len( mum *m )
{
    return m->len;
}
/**
 * Get the total length of the mum including additional sections
 * @param m the mum to measure
 * @return the total length of all segments
 */
int mum_total_len( mum *m )
{
    int len = 0;
    mum *temp = m;
    while ( temp != NULL )
    {
        len += temp->len;
        temp = temp->next;
    }
    return len;
}
card *mum_start_card( mum *m )
{
    return m->start.current;
}
card *mum_end_card( mum *m )
{
    return m->end.current;
}
bitset *mum_versions( mum *m )
{
    return m->bs;
}
int mum_text_off( mum *m )
{
    return m->text_off;
}
mum *mum_next( mum *m )
{
    return m->next;
}
int mum_text_end( mum *m )
{
    return m->text_off+m->len;
}
/**
 * Split the underlying card at mum start
 * @param m the mum in question
 * @param o the orphanage to store parents and children
 * @param log the log to write errors to
 * @return 1 if it worked
 */
static int mum_split_at_start( mum *m, orphanage *o, plugin_log *log )
{
    int res = 1;
    if ( m->start.pos==0 )
    {
        // ensure a node at the point of version-change
        if ( card_left(m->start.current)!=NULL && !card_node_to_left(m->start.current) )
        {
            pair *p = card_pair( m->start.current );
            bitset *bs = pair_versions( p );
            card *b = card_create_blank_bs(bs,NULL);
            card_add_before( m->start.current, b );
        }
    }
    else 
    {
        card *old_start = m->start.current;
        res = card_split( m->start.current, m->start.pos, o, log );
        if ( res )
        {
            m->start.current = card_right( m->start.current );
            // check if start and end were not the same
            if ( old_start == m->end.current )
            {
                m->end.current = m->start.current;
                m->end.pos -= m->start.pos;
            }
        }
    }
    m->start.pos = 0;
    return res;
}
/**
 * Split the underlying card at its end-point
 * @param m the mum in question
 * @param o the orphanage to store parents and children
 * @param log the log to write errors to
 * @return 1 if it worked
 */
static int mum_split_at_end( mum *m, orphanage *o, plugin_log *log )
{
    int res = 1;
    pair *p = card_pair(m->end.current);
    int plen = pair_len(p);
    if ( m->end.pos == plen-1 )
    {
        // ensure a node at the point of version-change
        if ( card_right(m->end.current)!=NULL && !card_node_to_right(m->end.current) )
        {
            pair *p = card_pair( m->end.current );
            bitset *bs = pair_versions( p );
            card *b = card_create_blank_bs(bs,NULL);
            card_add_after( m->end.current, b );
        }
    }
    else
    {
        res = card_split( m->end.current, m->end.pos+1, o, log );
    }
    return res;
}
/** 
 * Create a card containing an empty or short sequence in the new version
 * @param m1 the first mum before m2
 * @param m2 the second after m1
 * @param v the id of the new version
 * @return the between card or NULL if not needed
 */
static card *card_between_mums( mum *m1, mum *m2, UChar *text, int v, 
    plugin_log *log )
{
    card *between = NULL;
    int text_len = mum_text_off(m2)-mum_text_end(m1);
    UChar *fragment = NULL;
    if ( text_len > 0 )
    {
        fragment = calloc( text_len+1, sizeof(UChar) );
        if ( fragment != NULL )
        {
            memcpy( fragment, &text[mum_text_end(m1)], text_len*sizeof(UChar) );
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bs = bitset_set( bs, v );
                if ( bs != NULL )
                {
                    pair *frag = pair_create_basic( bs, fragment, text_len );
                    if ( frag != NULL )
                    {
                        between = card_create( frag, log );
                        if ( between != NULL )
                            card_set_text_off( between, mum_text_end(m1) );
                    }
                    bitset_dispose( bs );
                }
            }
            //card_print(between);
            free( fragment );
        }
        else
            plugin_log_add(log,"mum: failed to allocate in-between card");
    }
    return between;
}
/**
 * Split the MVD pairs corresponding to the mum. start_pos, end_pos are 
 * both inclusive
 * @param m the mum
 * @param text the text the mum is aligned with
 * @param v the number of the new version
 * @param o the orphanage to store parents and children
 * @param discards discarded cards array
 * @param log the log to write errors to
 * @return 1 if successful
 */
int mum_split( mum *m, UChar *text, int v, orphanage *o, 
    dyn_array *discards, plugin_log *log )
{
    mum *temp = m;
    int i,res=1;
    //mum_verify(m,v);
    dyn_array *mums = dyn_array_create(5);
    if ( mums != NULL )
    {
        // convert to an array because we need to go right-to-left
        dyn_array_add( mums, temp );
        while ( temp->next != NULL )
        {
            temp = temp->next;
            dyn_array_add( mums, temp );
        }
        for ( i=dyn_array_size(mums)-1;i>=0;i-- )
        {
            temp = dyn_array_get( mums, i );
            pair *p = card_pair(temp->end.current);
            res = mum_split_at_end( temp, o, log );
            if ( res ) 
                res = mum_split_at_start( temp, o, log );
            else
                plugin_log_add(log,"mum: failed to split mum at end\n");
            // special case for last card
            if ( card_right(temp->end.current) == NULL )
                temp->end.pos++;
            if ( i < dyn_array_size(mums)-1 )
            {
                // add a short segment linking to the following mum
                card *betw = card_between_mums(temp,
                    temp->next, text, v, log );
                if ( betw != NULL )
                {
                    res = dyn_array_add( discards, betw );
                }
            }
            if ( !res ) 
                break;
        }
        dyn_array_dispose( mums );
    }
    //mum_verify(m,v);
    return res;
}
/**
 * Work out if this mum is transposed
 * 1. find left and right closet direct alignments or the end 
 * 2. find the st_offset of each such alignment
 * 3. if our st_off is between these two, then it is direct, 
 * else it is transposed
 * @param m the mum to test
 * @param new_version the ID of the new version
 * @param tlen the overall length of the new version text
 * @param dist VAR param set to length of transposed distance
 * @return 1 if it is transposed else 0
 */
int mum_transposed( mum *m, int new_version, int tlen, int *dist )
{
    int text_left = 0;
    int text_right = tlen;
    // 1. find left direct align
    card *left = card_merged_left(m->start.current,new_version);
    if ( left != NULL )
        text_left = card_end( left );
    // find right direct align
    mum *last = m;
    while ( last->next != NULL )
        last = last->next;
    card *right = card_merged_right(last->end.current,new_version);
    if ( right != NULL )
        text_right = card_text_off( right );
    if ( m->text_off >= text_left && mum_text_end(last) <= text_right )
    {
        *dist = 0;
        return 0;
    }
    else
    {
        *dist = MIN(abs(m->text_off-text_left),abs(last->text_off-text_right));
        return 1;
    }
}
/**
 * Check that the mum is between aligned regions on each side
 * @param m the mum to check
 * @param v the new version of aligned sections
 * @return 1 if OK, else 0
 */
int mum_verify( mum *m, int v )
{
    int res = 1;
    card *c = m->start.current;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set(bs,v);
        while ( c != NULL )
        {
            c = card_left(c);
            if ( c != NULL )
            {
                pair *p = card_pair(c);
                if ( bitset_next_set_bit(pair_versions(p),v)==v )
                {
                    int end = card_text_off(c)+pair_len(p);
                    if ( end > m->text_off )
                    {
                        printf("mum: aligned card on left at %d > mum start %d\n",
                            end,m->text_off);
                        res = 0;
                    }
                    break;
                }
                c = card_left(c);
            }
        }
        c = m->end.current;
        while ( c != NULL )
        {
            c = card_right(c);
            if ( c != NULL )
            {
                pair *p = card_pair(c);
                if ( bitset_next_set_bit(pair_versions(p),v)==v )
                {
                    if ( card_text_off(c) < m->len+m->text_off )
                    {
                        printf("mum: aligned card on right at %d > mum start %d\n",
                            card_text_off(c),m->text_off+m->len);
                        res = 0;
                    }
                    break;
                }
                c = card_right(c);
            }
        }
    }
    return res;
}