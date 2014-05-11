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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/uchar.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "dyn_array.h"
#include "card.h"
#include "orphanage.h"
#include "hashmap.h"
#include "link_node.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define IMPLICIT_SIZE 12
static UChar ustring_empty[1] = {0};
//static hashmap *card_map=NULL;

/**
 * A card is necessary because when creating an MVD by using
 * only a list of fragments we need to insert new pairs. We can't
 * do this using an array without introducing inefficiencies and 
 * greatly complicating references to existing pairs by indices 
 * which may change all the time. So we wrap each pair in a 
 * 'card', which increases memory usage but solves the problem.
 * A card just implements a doubly-linked list of pairs.
 */
struct card_struct
{
    // the original pair from the mvd pairs array
    pair *p;
    // doubly-linked list
    card *left;
    card *right;
    // absolute offset in the suffixtree text if aligned to new version
    int text_off;
};
/*
 * Create a new card
 * @param p the pair to create
 * @param log the log to write errors to
 */
card *card_create( pair *p, plugin_log *log )
{
/*
    if ( card_map==NULL )
    {
        card_map = hashmap_create( 100, 1);
    }
*/
    card *c = calloc( 1, sizeof(card) );
    if ( c != NULL )
    {
        UChar key[16];
        c->p = p;
/*
        calc_ukey( key, (long)c, 16 );
        int res = hashmap_put( card_map, key, c );
        if ( !res )
            printf("Duplicate card!\n");
*/
    }
    else
        plugin_log_add( log, "card: failed to create object\n");
    return c;
}
/**
 * Create an empty card
 * @param version the version of the empty card
 * @param log the log to record errors in
 * @return an empty card
 */
card *card_create_blank( int version, plugin_log *log )
{
    card *c = NULL;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, version );
        pair *p = pair_create_basic( bs, ustring_empty, 0 );
        if ( p != NULL )
            c = card_create( p, log );
        else
            plugin_log_add(log,"card: failed to create pair");
        bitset_dispose( bs );
    }
    else
        plugin_log_add(log,"card: failed to create bitset");
    return c;
}
card *card_create_blank_bs( bitset *bs, plugin_log *log )
{
    card *c = NULL;
    pair *p = pair_create_basic( bs, ustring_empty, 0 );
    if ( p != NULL )
        c = card_create( p, log );
    else if ( log != NULL )
        plugin_log_add(log,"card: failed to create pair");
    return c;
}
/**
 * Just dispose of us
 * @param c the card object to free
 */
void card_dispose( card *c )
{
/*
    UChar key[16];
    calc_ukey( key, (long)c, 16 );
    int res = hashmap_remove( card_map, key, NULL );
    if ( !res )
        printf("card: failed to find card when freeing\n");
*/
    free( c );
}
/*
int card_exists( card *c )
{
    UChar key[16];
    calc_ukey( key, (long)c, 16 );
    return hashmap_contains( card_map, key );
}
*/
/**
 * Turn a card into a parent pair possibly with no children
 * @param c VAR param: the original card, maybe already a parent
 * @param log the log to record errors in
 * @return the parent or NULL if it failed
 */
card *card_make_parent( card **c, plugin_log *log )
{
    if ( pair_is_parent((*c)->p) )
        return *c;
    else if ( pair_is_child((*c)->p) )
        return NULL;
    else // ordinary card
    {
        card *oldc = *c;
        pair *pp = pair_create_parent( pair_versions(oldc->p), 
            pair_data(oldc->p), pair_len(oldc->p) );
        if ( pp != NULL )
        {
            card *parent = card_create(pp,log);
            // creates dangling parent - needs at least one child
            card_replace( oldc, parent );
            card_dispose( oldc );
            *c = parent;
            return parent;
        }
        else
            plugin_log_add(log,"card: failed to create parent pair\n");
        return NULL;
    }
}
/**
 * Create a child from the new version
 * @param parent the parent we are a child of
 * @param version the version of the child
 * @param log record errors here
 * @return the newborn child
 */
card *card_make_child( card *parent, int version, plugin_log *log )
{
    card *cc = NULL;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bs = bitset_set( bs, version );
        if ( bs != NULL )
        {
            pair *child = pair_create_child( bs );
            pair_add_child(card_pair(parent), child );
            bitset_dispose( bs );
            cc = card_create( child, log );
        }
        else
            plugin_log_add(log,"card: failed to create bitset\n");
    }
    return cc;
}
/**
 * Dispose of an entire list
 * @param c the card object to free
 */
void card_dispose_list( card *c )
{
    while ( c != NULL )
    {
        card *next = c->right;
        free( c );
        c = next;
    }
}
/**
 * Set the left pointer
 * @param c the card to set it for
 * @param left the next pair on the left
 */
void card_set_left( card *c, card *left )
{
    c->left = left;
}
/**
 * Set the right pointer
 * @param c the card to set it for
 * @param right the next pair on the right
 */
void card_set_right( card *c, card *right )
{
    c->right = right;
}
/**
 * Append a card to the very end of the list
 * @param a nominal card to start from (near the end)
 * @param after the card to put at the very end
 */
void card_append( card *c, card *after )
{
    while( c->right != NULL )
        c = c->right;
    c->right = after;
    after->left = c;
}
/**
 * Get the left pointer
 * @param c the card in question
 * @return the next pair on the left
 */
card *card_left( card *c )
{
    return c->left;
}
/**
 * Get the length of the underlying pair
 * @param c the card
 * @return the length of the data
 */
int card_len( card *c )
{
    return pair_len(c->p);
}
/**
 * Get the right pointer
 * @param c the card in question
 * @return the next pair on the right
 */
card *card_right( card *c )
{
    return c->right;
}
/**
 * Replace one card with another in the list
 * @param old_c the old card in a list
 * @param new_c the new card to add to the same list
 */
void card_replace( card *old_c, card *new_c )
{
    if ( old_c->left != NULL )
        old_c->left->right = new_c;
    if ( old_c->right != NULL )
        old_c->right->left = new_c;
    new_c->right = old_c->right;
    new_c->left = old_c->left;
    old_c->left = old_c->right = NULL;
}
/**
 * Set the index into the suffixtree text
 * @param c the card to set it for
 * @param text_off the index
 */
void card_set_text_off( card *c, int text_off )
{
    c->text_off = text_off;
}
/**
 * Get the index into the suffixtree text
 * @param c the card to set it for
 * @return the index
 */
int card_text_off( card *c )
{
    return c->text_off;
}
/**
 * Get the new version text position one past the end of the card
 * @param c the card to get the end of
 * @return the end+1. So for a card of length 6 starting at 0, 6
 */
int card_end( card *c )
{
    return c->text_off+pair_len(c->p);
}
/**
 * Get the pair associated with this card
 * @param c the card
 * @return its pair
 */
pair *card_pair( card *c )
{
    return c->p;
}
/**
 * Is a card the trailing arc of a node (or a hint)?
 * @param c the card to test
 * @return 1 if it is, else 0
 */
int card_trailing_node( card *c )
{
    if ( c->left != NULL )
    {
        if ( pair_is_hint(c->left->p)
            || bitset_intersects(pair_versions(c->left->p),
            pair_versions(c->p)) )
            return 1;
    }
    return 0;
}
/**
 * Get the next card on the right with a given version and at least one other
 * @param c the card to start from
 * @param version the version id
 * @return the card fulfilling the requirement or NULL
 */
card *card_merged_right( card *c, int version )
{
    card *right = c->right;
    while ( right != NULL )
    {
        pair *p = card_pair( right );
        bitset *bs = pair_versions(p);
        if ( bitset_next_set_bit(bs,version)==version 
            /*&& bitset_cardinality(bs)>1*/ )
        {
            break;
        }
        right = right->right;
    }
    return right;
}
/**
 * Get the next card on the left with a given version and at least one other
 * @param c the card to start from
 * @param version the version id
 * @return the card fulfilling the requirement or NULL
 */
card *card_merged_left( card *c, int version )
{
    card *left = c->left;
    while ( left != NULL )
    {
        pair *p = card_pair( left );
        bitset *bs = pair_versions(p);
        if ( bitset_next_set_bit(bs,version)==version 
            /*&& bitset_cardinality(bs)>1*/ )
        {
            break;
        }
        left = left->left;
    }
    return left;
}
/**
 * Get the next pair in a list
 * @param pairs a list of cards
 * @param c the first card to look at
 * @param bs the bitset of versions to follow
 * @param avoid avoid this version
 * @return NULL on failure else the the next relevant card
 */
card *card_next( card *c, bitset *bs, int avoid )
{
    c = c->right;
    while ( c != NULL )
    {
        bitset *pv = pair_versions(card_pair(c));
        if ( avoid != 0 && bitset_next_set_bit(pv,avoid)==avoid )
        {
            c = NULL;
            break;
        }
        else if ( bitset_intersects(bs,pv) )
            break;
        else
            c = card_right(c);
    }
    return c;
}
/**
 * Get the first card in the list that intersects with the given versions
 * @param list the list of cards
 * @param bs the set of versions to search for
 * @return the first card in list that intersects with bs or NULL
 */
card *card_first( card *list, bitset *bs )
{
    bitset *pv = NULL;
    while ( list != NULL ) 
    {
        pair *p = card_pair(list);
        pv = pair_versions(p);
        if ( !bitset_intersects(pv,bs) )
            list = list->right;
        else
            break;
    }
    return list;
}
/**
 * Get the previous pair in a list
 * @param pairs a list of cards
 * @param c the first card to look at
 * @param bs the bitset of versions to follow
 * @return NULL on failure else the the previous relevant card
 */
card *card_prev( card *c, bitset *bs )
{
    c = c->left;
    while ( c != NULL 
        && !bitset_intersects(bs,pair_versions(card_pair(c))) )
        c = card_left(c);
    return c;
}
/**
 * Get the next nonempty card
 * @param c the card to start from
 * @param bs the bitset to look for
 * @param avoid avoid this version
 * @return the next nonempty card or NULL
 */
card *card_next_nonempty( card *c, bitset *bs, int avoid )
{
    do
    {
        c = card_next( c, bs, avoid );
    }
    while (c != NULL && pair_len(card_pair(c))==0 );
    return c;
}
card *card_prev_nonempty( card *c, bitset *bs )
{
    do
    {
        c = card_prev( c, bs );
    }
    while ( c != NULL && pair_len(card_pair(c))==0 );
    return c;
}
/**
 * Compute the overlap between the versions of this card and another
 * @param c the current card
 * @param bs the versions of the other card that may overlap those in c
 * @return an allocated set of bits in bs that not in c, or NULL if none
 */
bitset *card_overlap( card *c, bitset *bs )
{
    bitset *overlap = NULL;
    bitset *p_versions = pair_versions( c->p );
    if ( !bitset_equals(bs,p_versions) )
    {
        overlap = bitset_clone( bs );
        bitset_and_not( overlap, p_versions );
        if ( bitset_empty(overlap) )
        {
            bitset_dispose( overlap );
            overlap = NULL;
        }
    }
    return overlap;
}
/**
 * Is this card free? i.e. is it not the trailing pair of a node?
 * @param c the card to test
 * @return 1 if it is free else 0
 */
int card_free( card *c )
{
    int res = 1;
    if ( c->left != NULL )
    {
        pair *p = card_pair(c->left);
        if ( !pair_is_hint(p) )
        {
            pair *q = card_pair(c);
            //char str1[34];
            //char str2[34];
            //bitset *bs1 = pair_versions(p);
            //bitset *bs2 = pair_versions(q);
            //bitset_tostring(bs1,str1,34);
            //bitset_tostring(bs2,str2,34);
            res = !bitset_intersects(pair_versions(p),pair_versions(q));
        }
    }
    return res;
}
/**
 * Split a single card without regard to anything it may be attached to.
 * @param c the card to split.
 * @param at point before which to split
 * @param o the orphanage where all the parents and children hang out
 * @param log record failures in the log
 * @return 1 if it worked else 0
 */
static int card_split_one( card *c, int at, orphanage *o, plugin_log *log )
{
    int res = 1;
    // q is the new trailing pair
    int is_parent = pair_is_parent( c->p );
    int id = (is_parent)?pair_id(c->p):0;
    pair *q = pair_split( &c->p, at );
    if ( q != NULL )
    {
        card *c2 = card_create( q, log );
        if ( c2 != NULL )
        {
            card_set_text_off( c2, card_end(c) );
            card_add_after(c,c2);
        }
        else
            res = 0;
        if ( is_parent )
        {
            pair_set_id( c->p,id );
            int id2 = orphanage_next_id( o );
            pair_set_id( q, id2 );
        }
    }
    else
    {
        plugin_log_add(log,"failed to split pair\n");
        res = 0;
    }
    return res;
}
/**
 * Split a parent card and all its children
 * @param p1 the card to split
 * @param at the position before which to split it
 * @param o the orphanage where all the children and parents are
 * @param log the log to record errors
 * @return 1 if it worked else 0
 */
static int card_split_parent( card *p1, int at, orphanage *o, plugin_log *log )
{
    int res = 0;
    if ( at < pair_len(card_pair(p1)) )
    {
        int i,size = orphanage_count_children( o, p1 );
        card **children=calloc( size, sizeof(card*) );
        if ( children != NULL )
        {
            orphanage_get_children( o, p1, children, size );
            res = orphanage_remove_parent( o, p1 );
        }
        if ( res )
        {
            // split the parent
            res = card_split_one( p1, at, o, log );
            if ( res )
            {
                card *p2 = card_right(p1);
                // now split the children
                for ( i=0;i<size;i++ )
                {
                    res = card_split_one( children[i], at, o, log );
                    if ( res ) 
                    {
                        card *c2 = card_right( children[i] );
                        pair *ch1 = pair_add_child( card_pair(p1), 
                            card_pair(children[i]) );
                        pair *ch2 = pair_add_child( card_pair(p2), 
                            card_pair(c2) );
                        pair_set_id( ch1, pair_id(card_pair(p1)) );
                        pair_set_id( ch2, pair_id(card_pair(p2)) );
                        if ( ch1 == NULL || ch2 == NULL )
                            res = 0;
                        if ( res )
                            res = orphanage_add_child( o, children[i] );
                        if ( res )
                            res = orphanage_add_child( o, c2 );
                    }
                    if ( !res )
                        break;
                }
                // this will gather the children
                if ( res )
                    res = orphanage_add_parent( o, p1 );
                if ( res )
                    res = orphanage_add_parent( o, p2 );
            }
        }
    }
    return res;
}
/**
 * Split a card and adjust parent/child if not a basic pair
 * @param c the card to split
 * @param at the offset in the pair data BEFORE which to split
 * @param o the orphanage where all the parents and children are
 * @param log the log to record errors in
 * @return 1 if it split successfully
 */
int card_split( card *c, int at, orphanage *o, plugin_log *log )
{
    int res = 0;
    if ( at > 0 && at < pair_len(c->p) )
    {
        if ( pair_is_ordinary(c->p) )
        {
            res = card_split_one(c,at,o,log);
        }
        else if ( pair_is_child(c->p) )
        {
            card *parent = orphanage_get_parent(o,c);
            if ( parent != NULL )
                res = card_split_parent(parent,at,o,log);
            else
                res = 0;
        }
        else if ( pair_is_parent(c->p) )
        {
            res = card_split_parent(c,at,o,log);
        }
    }
    return res;
}
/**
 * Convert a list of cards to a standard pair array
 * @param c the head of the card list
 * @return an allocated dynamic array of pairs or NULL
 */
dyn_array *card_to_pairs( card *c )
{
    int npairs = card_list_len(c);
    dyn_array *da = dyn_array_create( npairs );
    if ( da != NULL )
    {
        while ( c != NULL )
        {
            dyn_array_add( da, c->p );
            c = c->right;
        }
    }
    return da;
}
/**
 * Convert a card to a utf-8 string
 * @param c the card to stringify
 * @return an allocated string - please dispose after use!
 */
char *card_tostring( card *c )
{
    char *str = NULL;
    char vbs[32];
    char *udata;
    int dlen = 0;
    pair *p = card_pair(c);
    bitset *bs = pair_versions(p);
    UChar *data = pair_data( p );
    int highest = bitset_top_bit( bs );
    bitset_tostring( bs, vbs, highest+2 );
    if ( pair_len(p) > 0 )
    {
        dlen = measure_to_encoding( data, pair_len(p), "utf-8" );
        udata = calloc( dlen+1, 1 );
        if ( udata != NULL )
            convert_to_encoding( data, pair_len(p), udata, dlen+1, "utf-8" );
    }
    else
        udata = calloc( 1, sizeof(UChar) );
    if ( udata != NULL )
    {
        char tid[16];
        if ( pair_is_parent(p) )
            snprintf(tid,16,"-p:%d",pair_id(p));
        else if ( pair_is_child(p) )
            snprintf(tid,16,"-c:%d",pair_id(p));
        else
            tid[0] = 0;
        int tlen = strlen(vbs)+dlen+strlen(tid)+3;
        str = calloc( tlen, 1 );
        if ( str != NULL )
            snprintf( str, tlen, "[%s%s]%s", vbs, tid, udata );
        free( udata );
    }
    return str;
}
/**
 * Do we define a node immediately on our right?
 * @param c the card in question
 * @return 1 if we do else 0
 */
int card_node_to_right( card *c )
{
    int res = 0;
    if ( c->right != NULL )
    {
        if ( pair_is_hint(c->right->p) )
            res = 1;
        else
        {
            bitset *bs1 = pair_versions(c->p);
            bitset *bs2 = pair_versions(c->right->p);
            res = bitset_intersects(bs1,bs2);
        }
    }
    return res;
}
/**
 * Do we define a node immediately on our left?
 * @param c the card in question
 * @return 1 if we do else 0
 */
int card_node_to_left( card *c )
{
    int res = 0;
    if ( c->left != NULL )
    {
        if ( pair_is_hint(c->left->p) )
            res = 1;
        else
        {
            bitset *bs1 = pair_versions(c->p);
            bitset *bs2 = pair_versions(c->left->p);
            res = bitset_intersects(bs1,bs2);
        }
    }
    return res;
}
/**
 * Compute the overhang for a card node
 * @param c the leading pair of a node
 * @return the bitset of the overhang or NULL (user to free)
 */
bitset *card_node_overhang( card *c )
{
    bitset *bs = bitset_create();
    bs = bitset_or( bs, pair_versions(c->p) );
    if ( pair_is_hint(c->right->p) )
    {
        pair *pp = c->right->p;
        bs = bitset_or( bs, pair_versions(pp) );
        if ( pair_is_hint(pp) )
            bitset_clear_bit(bs,0);
        c = c->right;
    }
    bitset_and_not( bs, pair_versions(c->right->p) );
    return bs;
}
/**
 * Add a card AFTER a node. c->right will be displaced by one 
 * card. Although this will increase the node's overhang, the increase
 * is exactly that of the displaced card's versions, which will reattach 
 * via the overhang, instead of the forced rule.
 * @param c the incoming pair of a node
 * @param after the card to add into lp's node. must intersect with lp.
 * @param verify if 1 then check that the resulting node is OK
 * @return 1 if the new node is a bona fide node
 */
int card_add_at_node( card *c, card *after, int verify )
{
    int res = 0;
    int dispose = 0;
    bitset *bs1 = pair_versions(c->p);
    if ( pair_is_hint(card_pair(c->right)) ) 
    {
        bs1 = bitset_clone(bs1);
        if ( bs1 != NULL )
        {
            c = c->right;
            dispose = 1;
            // bs1 is the amalgam of leading pair+hint
            bitset_or( bs1, pair_versions(c->p) );
        }
    }
    if ( bs1 != NULL )
    {
        after->right = c->right;
        after->left = c;
        if ( c->right != NULL )
            c->right->left = after;
        c->right = after;
        res = (verify)?bitset_intersects(bs1,pair_versions(after->p)):1;
        if ( dispose )
            bitset_dispose( bs1 );
    }
    return res;
}
int card_is_blank( card *c )
{
    return pair_len(c->p)==0;
}
static int card_merge_left( card *c_new, card *c )
{
    int merged = 0;
    if ( card_is_blank(c_new) )
    {
        bitset *cv= pair_versions(c_new->p);
        card *l = card_prev_nonempty(c,cv);
        card *temp = c;
        while ( temp != NULL && temp != l )
        {
            bitset *tv = pair_versions(temp->p);
            if ( card_is_blank(temp) && card_prev(temp,tv)==l )
            {
                bitset_or(tv,cv);
                merged = 1;
                break;
            }
            temp = temp->left;
        }
    }
    return merged;
}
/**
 * Find a blank before the next node that we can add our versions to
 * @paramc_new the new blank card
 * @param c the card to start searching from
 * @return 1 we merged c_new into a blank card already there else 0
 */
static int card_merge_right( card *c_new, card *c )
{
    int merged = 0;
    if ( card_is_blank(c_new) )
    {
        bitset *cv= pair_versions(c_new->p);
        card *r = card_next_nonempty(c,cv,0);
        if ( r != NULL )
        {
            pair *pr = card_pair(r);
            bitset *rv = pair_versions(pr);
            card *temp = c->right;
            while ( temp != NULL && temp != r )
            {
                bitset *tv = pair_versions(temp->p);
                if ( card_is_blank(temp) && bitset_intersects(tv,rv) )
                {
                    bitset_or(tv,cv);
                    merged = 1;
                    break;
                }
                temp = temp->right;
            }
        }
    }
    return merged;
}
/**
 * Add a new card before an existing one
 * @param c the existing card
 * @param c_new the new one
 */
void card_add_before( card *c, card *c_new )
{
    if ( !card_merge_left(c_new, c) )
    {
        if ( c->left == NULL )
        {
            c_new->right = c->right;
            c->left = c_new;
        }
        else
        {
            c_new->right = c->right;
            c_new->left = c->left;
            c->right = c_new;
            if ( c->right != NULL )
                c->right->left = c_new;
        }
    }
}
/**
 * Compute the number of incoming arcs to a node
 * @param rhs the card on the rhs of the node
 * @return the number of cards oined as incoming to the node
 */
static int card_in_degree( card *rhs )
{
    int ind = 0;
    card *lhs = rhs->left;
    if ( lhs != NULL )
    {
        pair *pr = card_pair( rhs );
        bitset *bs = bitset_clone( pair_versions(pr) );
        if ( bs != NULL )
        {
            pair *pl = card_pair( lhs );
            if ( bitset_intersects(pair_versions(pr),pair_versions(pl)) )
            {
                ind = 1;
                bitset_and_not( bs, pair_versions(pl) );
                card *temp = lhs;
                while ( temp != NULL && !bitset_empty(bs) )
                {
                    temp = temp->left;
                    if ( temp != NULL )
                    {
                        pair *pt = card_pair(temp);
                        if ( bitset_intersects(bs,pair_versions(pt)) )
                        {
                            bitset_and_not( bs,pair_versions(pt) );
                            ind++;
                        }
                    }
                }
            }
            bitset_dispose( bs );
        }
    }
    return ind;
}
/**
 * Compute the extra blank sometimes needed after deletion blanks
 * @param c the card after which the deletion blank will be inserted
 * @param after the deletion blank not yet inserted
 * @return the set of versions or NULL needed for the extra blank
 */
bitset *card_compute_extra_blank( card *c, card *after )
{
    bitset *blank = NULL;
    pair *p = card_pair( c );
    bitset *bs = pair_versions( p );
    card *rhs = card_next( c, bs, 0 );
    if ( rhs != NULL && card_in_degree(rhs)>1 )
    {
        blank = bitset_clone( bs );
        if ( blank != NULL )
            bitset_and_not( blank, pair_versions(card_pair(after)) );
    }
    return blank;
}
/**
 * Add a card after a given point, creating a new node.
 * @param c the point to add it after
 * @param after the card to become the one after lp
 * @return 1 if successful else 0
 */
void card_add_after( card *c, card *after )
{
    if ( !card_merge_right(after,c) )
    {
        after->right = c->right;
        after->left = c;
        if ( c->right != NULL )
            c->right->left = after;
        c->right = after;
    }
}
/**
 * Add a new card
 * @param list the list of cards to add it to
 * @param c the card to add
 * @param text_off the offset in version for lp
 * @param version the version to follow
 */
void card_add_at( card **list, card *c, int text_off, int version )
{
    card *temp = *list;
    bitset *bs = bitset_create();
    if ( bs != NULL )
        bs = bitset_set( bs, version );
    if ( bs != NULL )
    {
        card *last = NULL;
        while ( temp != NULL )
        {
            pair *p = card_pair(temp);
            if ( bitset_next_set_bit(pair_versions(p),version)== version )
            {
                int off = card_text_off(temp);
                if ( off < text_off )
                    last = temp;
                else
                {
                    if ( last != NULL )
                        card_add_after( last, c );
                    else
                    {
                        c->right = *list;
                        *list = c;
                    }
                    break;
                }
            }
            temp = card_next( temp, bs, 0 );
        }
        bitset_dispose( bs );
    }
}
/**
 * Measure the length of a list
 * @param c the list to measure
 * @return its length in cards
 */
int card_list_len( card *c )
{
    int len = 0;
    while ( c != NULL )
    {
        len++;
        c = c->right;
    }
    return len;
}
/**
 * Check if a list is broken by being circular
 * @param c a card pointer somewhere in the list
 * @return 1 if it has two ends else 0
 */
int card_list_circular( card *c )
{
    int res = 0;
    hashmap *hm = hashmap_create( 12, 0 );
    card *right = c->right;
    char junk[16];
    UChar key[32];
    char ckey[32];
    strcpy( junk, "junk" );
    snprintf(ckey,32,"%lx",(long)right);
    u_print( key, ckey, strlen(ckey) );
    hashmap_put( hm, key, junk );
    while ( !res && right != NULL )
    {
        snprintf(ckey,32,"%lx",(long)right);
        //printf("%s\n",ckey);
        ascii_to_uchar( ckey, key, 32 );
        if ( hashmap_contains(hm,key) )
            res = 1;
        else
        {
            hashmap_put( hm, key, junk );
            right = right->right;
        }
    }
    card *left = c->left;
    while ( !res && left != NULL )
    {
        snprintf(ckey,32,"%lx",(long)left);
        //printf("%s\n",ckey);
        ascii_to_uchar( ckey, key, 32 );
        if ( hashmap_contains(hm,key) )
            res = 1;
        else
        {
            hashmap_put( hm, key, junk );
            left = left->left;
        }
    }
    hashmap_dispose( hm, NULL );
    return res;
}
/**
 * Remove a card, disposing of it and delinking it from the list
 * @param c the card to remove
 * @param dispose if 1 dispose of the removed card
 */
void card_remove( card *c, int dispose )
{
    if ( c->left != NULL )
        c->left->right = c->right;
    if ( c->right != NULL )
        c->right->left = c->left;
    if ( dispose )
        card_dispose( c );
}
/**
 * Debug
 */
void card_print_until( card *from, card *to )
{
    card *c = from;
    while ( c != NULL )
    {
        if ( c == from )
            printf("[from]");
        else if ( c == to )
            printf("[to]");
        printf("text_off: %d len: %d",c->text_off,card_len(c));
        pair *p = card_pair(c);
        pair_print( p );
        if ( c == to )
            break;
        c = c->right;
    }
}
void card_print( card *c )
{
    while ( c != NULL )
    {
        printf("text_off: %d len: %d",c->text_off,card_len(c));
        pair *p = card_pair(c);
        if ( card_len(c)==1 )
        {
            UChar *data = pair_data(p);
            int i = (int)(*data);
            printf("[%d]",i);
        }
        pair_print( p );
        c = c->right;
    }
}
void card_print_list( card *list )
{
    card *c = list;
    while ( c != NULL )
    {
        char *str = card_tostring( c );
        if ( str!= NULL )
        {
            printf("%s",str );
            free( str );
            c = card_right( c );
            if ( c != NULL )
                printf("->");
            else
                printf("\n");
        }
    }
}
#ifdef MVD_TEST
static UChar data1[7] = {'b','a','n','a','n','a',0};
static UChar data2[6] = {'a','p','p','l','e',0};
static UChar data3[5] = {'p','e','a','r',0};
static UChar data4[7] = {'o','r','a','n','g','e',0};
static UChar data5[6] = {'g','u','a','v','a',0};
static UChar data6[8] = {'c','u','m','q','u','a','t',0};
static bitset *bs1,*bs2,*bs3,*bs4,*bs5,*bsr,*bsl,*bsm;
static card *c1,*c2,*c3,*c4,*cl,*cr,*cc;
static pair *p1,*p2,*p3,*p4,*pl,*pr,*pc;
static int make_test_data( plugin_log *log )
{
    c1 = c2 = c3 = c4 = cr = cl = cc = NULL;
    bs1 = bs2 = bs3 = bs4 = bs5 = bsl = bsr = bsm = NULL;
    p1 = p2 = p3 = p4 = pl = pr = pc = NULL;
    bs1 = bitset_create();
    bitset_set(bs1,3);
    bitset_set(bs1,27);
    bs2 = bitset_create();
    bitset_set(bs2,5);
    bitset_set(bs2,17);
    bs3 = bitset_create();
    bitset_set(bs3,7);
    bitset_set(bs3,14);
    bitset_set(bs3,19);
    bs4 = bitset_create();
    bitset_set(bs4,7);
    bitset_set(bs4,14);
    bitset_set(bs4,22);
    bs5 = bitset_create();
    bitset_set(bs5,7);
    bitset_set(bs5,14);
    bsl = bitset_create();
    bsr = bitset_create();
    bsl = bitset_set(bsl,3);
    if ( bsl != NULL )
        bsl = bitset_set(bsl,23);
    bsr = bitset_set(bsr,3);
    if ( bsr != NULL )
        bsr = bitset_set(bsr,21);
    bsm = bitset_create();
    bitset_set(bsm,23);
    bitset_set(bsm,7);
    if ( bs1 != NULL && bs2 != NULL && bs3 != NULL && bs4 != NULL 
        && bs5 != NULL && bsl != NULL && bsr != NULL && bsm != NULL )
    {
        p1 = pair_create_basic(bs1, data1, 6);
        p2 = pair_create_basic(bs2, data2, 5);
        p3 = pair_create_basic(bs3, data3, 4);
        p4 = pair_create_basic(bs4, data4, 6);
        pl = pair_create_basic(bsl,data4,6);
        pr = pair_create_basic(bsr,data5,5);
        pc = pair_create_basic(bsm,data6,7);
        if ( p1 != NULL && p2 != NULL && p3 != NULL && p4 != NULL 
            && pl != NULL && pr != NULL && pc != NULL )
        {
            c1 = card_create( p1, log );
            c2 = card_create( p2, log );
            c3 = card_create( p3, log );
            c4 = card_create( p4, log );
            cl = card_create( pl, log );
            cr = card_create( pr, log );
            cc = card_create( pc, log );
            if ( c1 != NULL && c2 != NULL && c3 != NULL && c4 != NULL 
                && cl != NULL && cr != NULL && cc != NULL )
                return 1;
        }
    }
    return 0;
}
static void free_test_data( plugin_log *log )
{
    if ( c1 != NULL )
        card_dispose( c1 );
    if ( c2 != NULL )
        card_dispose( c2 );
    if ( c3 != NULL )
        card_dispose( c3 );
    if ( c4 != NULL )
        card_dispose( c4 );
    if ( cl != NULL )
        card_dispose( cl );
    if ( cr != NULL )
        card_dispose( cr );
    if ( cc != NULL )
        card_dispose( cc );
    if ( p1 != NULL )
        pair_dispose( p1 );
    if ( p2 != NULL )
        pair_dispose( p2 );
    if ( p3 != NULL )
        pair_dispose( p3 );
    if ( p4 != NULL )
        pair_dispose( p4 );
    if ( pl != NULL )
        pair_dispose( pl );
    if ( pr != NULL )
        pair_dispose( pr );
    if ( pc != NULL )
        pair_dispose( pc );
    if ( bs1 != NULL )
        bitset_dispose( bs1 );
    if ( bs2 != NULL )
        bitset_dispose( bs2 );
    if ( bs3 != NULL )
        bitset_dispose( bs3 );
    if ( bs4 != NULL )
        bitset_dispose( bs4 );
    if ( bs5 != NULL )
        bitset_dispose( bs5 );
    if ( bsl != NULL )
        bitset_dispose( bsl );
    if ( bsr != NULL )
        bitset_dispose( bsr );
    if ( bsm != NULL )
        bitset_dispose( bsm );
    if ( log != NULL )
        plugin_log_dispose( log );
}
static void card_test_links( int *passed, int *failed, plugin_log *log )
{
    card_set_left( c2, c1 );
    card_set_right( c1, c2 );
    card_set_right( c2, c3 );
    card_set_left( c3, c2 );
    if ( card_right(c2) != c3 && card_left(c3) != c2 )
    {
        fprintf(stderr,"card: card_right failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
    if ( card_left(c2) != c1 && card_right(c1) != c2 )
    {
        fprintf(stderr,"card: card_left failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_replace( int *passed, int *failed, plugin_log *log )
{
    int len1 = card_list_len(c1);
    card_replace( c2, c4 );
    if ( card_right(c4) != c3 || card_left(c4) != c1 )
    {
        (*failed)++;
        fprintf(stderr,"card: card_replace failed\n");
    }
    else
        (*passed)++;
    if ( card_pair(c1)!=p1 || card_pair(c2)!=p2
        || card_pair(c3)!=p3 || card_pair(c4)!=p4 )
    {
        fprintf(stderr,"card: pair not set correctly\n");
        (*failed)++;
    }
    else
        (*passed)++;
    int len2 = card_list_len(c1);
    if ( len1 != len2 )
    {
        fprintf(stderr,"card: replace failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_text_off( int *passed, int *failed, plugin_log *log )
{
    card_set_text_off( c2, 123 );
    if ( card_text_off(c2) != 123 )
    {
        fprintf(stderr,"card: card_set_text_off failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_trailing( int *passed, int *failed, plugin_log *log )
{
    if ( !card_trailing_node(c3) )
    {
        fprintf(stderr,"card: not a trailing node\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_next( int *passed, int *failed, plugin_log *log )
{
    card *next = card_next(c1,bs5,0);
    if ( next != c4 )
    {
        fprintf(stderr,"card: card_next failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_free( int *passed, int *failed, plugin_log *log )
{
    if ( !card_free(c4) )
    {
        fprintf(stderr,"card: card_free failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_split( int *passed, int *failed, orphanage *o, plugin_log *log )
{
    card_split( c2, 2, o, log );
    card *right = card_right(c2);
    if ( right != NULL )
    {
        int len1 = pair_len(card_pair(c2));
        int len2 = pair_len(card_pair(right));
        if ( len1!=2 ||len2 !=3 )
        {
            fprintf(stderr,"card: split failed\n");
            (*failed)++;
        }
        else
            (*passed)++;
        pair_dispose( card_pair(right) );
        card_dispose( right );
        c2->right = NULL;
    }
    else
    {
        fprintf(stderr,"card: split failed\n");
        (*failed)++;
    }
}
static void card_test_array( int *passed, int *failed, plugin_log *log )
{
    dyn_array *array = card_to_pairs( c1 );
    if ( array != NULL )
    {
        int i;
        card *c = c1;
        for ( i=0;i<dyn_array_size(array);i++,
            c=card_right(c) )
        {
            pair *p = dyn_array_get(array,i);
            if ( p != card_pair(c) )
            {
                (*failed)++;
                fprintf(stderr,
                    "card: list to array failed 1\n");
                break;
            }
        }
        if ( c != NULL )
        {
            (*failed)++;
            fprintf(stderr,"card: list to array failed 2\n");
        }
        dyn_array_dispose( array );
    }
    else
    {
        fprintf(stderr,"card: list to array failed 3\n");
        (*failed)++;
    }
}
static void card_test_node_to_right( int *passed, int *failed, plugin_log *log )
{
    card_set_right(cl,cr);
    card_set_left(cr,cl);
    if ( !card_node_to_right(cl) )
    {
        fprintf(stderr,"card: failed test node to right 1\n");
        (*failed)++;
    }
    else
    {
        (*passed)++;
    }
}
static void card_test_node_to_left( int *passed, int *failed, plugin_log *log )
{
    card_set_right(cl,cr);
    card_set_left(cr,cl);
    if ( !card_node_to_left(cr) )
    {
        fprintf(stderr,"card: failed test node to left 1\n");
        (*failed)++;
    }
    else
    {
        (*passed)++;
    }
}
static void card_test_overhang( int *passed, int *failed, plugin_log *log )
{
    card_set_right(cl,cr);
    card_set_left(cr,cl);
    bitset *bs = card_node_overhang( cl );
    if ( bs != NULL )
    {
        bitset *bsc = bitset_create();
        if ( bsc != NULL )
        {
            bsc = bitset_or( bsc, bsl );
            bsc = bitset_set( bsc, 7 );
            bitset_and_not( bsc, pair_versions(pr) );
            if ( !bitset_equals(bsc,bs) )
            {
                fprintf(stderr,"card: overhang text failed\n");
                (*failed)++;
            }
            else
                (*passed)++;
        }
        else
            (*failed)++;
        if ( bsc != NULL )
            bitset_dispose( bsc );
    }
    else
    {
        fprintf(stderr,"card: failed to compute overhang\n");
        (*failed)++;
    }
    if ( bs != NULL )
        bitset_dispose( bs );
}
static void card_test_add_at_node( int *passed, int *failed, 
    plugin_log *log )
{
    card_set_right(cl,cr);
    card_set_left(cr,cl);
    int res = card_add_at_node( cl, cc, 1 );
    if ( res )
    {
        (*passed)++;
        if ( !card_node_to_right(cl) || !card_node_to_left(cc) 
            || card_node_to_left(cr) )
        {
            fprintf(stderr,"card: add_at_node failed 1\n");
            (*failed)++;
        }
        else
            (*passed)++;
        int len = card_list_len(cl);
        if ( len != 3 )
        {
            fprintf(stderr,"card: length of add_at_node list wrong\n");
            (*failed)++;
        }
        else
            (*passed)++;
    }
    else
    {
        (*failed)++;
        fprintf(stderr,"card: add_at_node failed 2\n");
    }
}
static void card_test_add_after( int *passed, int *failed, plugin_log *log )
{
    cr->right = cr->left = NULL;
    cl->left = cl->right = NULL;
    cc->left = cc->right = NULL;
    card_add_after( cl, cr );
    if ( card_right(cl)!=cr )
    {
        fprintf(stderr,"card: failed to add after\n");
        (*failed)++;
    }
    else
    {
        (*passed)++;
        card_add_after(cl,cc);
        if ( card_right(cl)!=cc )
        {
            fprintf(stderr,"card: add_after should have failed\n");
            (*failed)++;
        }
        else
            (*passed)++;
    }
}
static void card_test_list_len( int *passed, int *failed, plugin_log *log )
{
    card_set_right(c1,c2);
    card_set_left(c2,c1);
    card_set_right(c2,c3);
    card_set_left(c3,c2);
    card_set_right(c3,c4);
    card_set_left(c4,c3);
    card_set_right(c4,NULL);
    int len = card_list_len( c1 );
    if ( len != 4 )
    {
        fprintf(stderr,"card: list length failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_circular( int *passed, int *failed, plugin_log *log )
{
    card_set_right(c1,c2);
    card_set_left(c2,c1);
    card_set_right(c2,c3);
    card_set_left(c3,c2);
    card_set_right(c3,c4);
    card_set_left(c4,c3);
    card_set_right(c4,c1);
    int res = card_list_circular( c1 );
    if ( !res )
    {
        fprintf(stderr,"card: circular test failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
static void card_test_remove( int *passed, int *failed, plugin_log *log )
{
    card_set_right(c1,c2);
    card_set_left(c2,c1);
    card_set_right(c2,c3);
    card_set_left(c3,c2);
    card_set_right(c3,c4);
    card_set_left(c4,c3);
    card_set_right(c4,NULL);
    int len1 = card_list_len(c1);
    card_remove( c3, 0 );
    int len2 = card_list_len(c1 );
    if ( len1-len2 != 1 )
    {
        fprintf(stderr,"card: remove failed\n");
        (*failed)++;
    }
    else
        (*passed)++;
}
void card_test( int *passed, int *failed )
{
    plugin_log *log = plugin_log_create();
    if ( log != NULL )
    {
        int res = make_test_data(log);
        if ( res )
        {
            orphanage *o = orphanage_create();
            card_test_links( passed, failed, log );
            card_test_replace( passed, failed, log );
            card_test_text_off( passed, failed, log );    
            card_test_trailing( passed, failed, log );
            card_test_next( passed, failed, log );
            card_test_split( passed, failed, o, log ); 
            card_test_array( passed, failed, log );
            card_test_node_to_right( passed, failed, log );
            card_test_node_to_left( passed, failed, log );
            card_test_overhang( passed, failed, log );
            card_test_add_at_node( passed, failed, log );
            card_test_add_after( passed, failed, log );
            card_test_list_len( passed, failed, log );
            card_test_circular( passed, failed, log );
            card_test_remove( passed, failed, log );
        }
        else
        {
            fprintf(stderr,"card: failed to initialise test data\n");
            (*failed)++;
        }
    }
    else
        (*failed)++;
    free_test_data(log);
}
int card_compare( void *c1, void *c2 )
{
    card *cd1 = c1;
    card *cd2 = c2;
    if ( cd1->text_off < cd2->text_off )
        return -1;
    else if ( cd1->text_off == cd2->text_off )
        return 0;
    else
        return 1;
}
#endif