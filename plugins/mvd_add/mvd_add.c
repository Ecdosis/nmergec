/*
 *  NMergeC is Copyright 2014 Desmond Schmidt
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/resource.h>
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "pair.h"
#include "version.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "encoding.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "hashmap.h"
#include "utils.h"
#include "option_keys.h"
#include "card.h"
#include "orphanage.h"
#include "location.h"
#include "match.h"
#include "aatree.h"
#include "orphanage.h"
#include "mum.h"
#include "alignment.h"
#include "deck.h"
#include "verify.h"
#include "adder.h"
#include "benchmark.h"
#include "vgnode.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define ALIGNMENTS_LEN 100 
#define PARENT "[p]"
/**
 * Add the first version to an mvd
 * @param mvd the mvd to add it to
 * @param add the object representing this mvd_add operation
 * @param text the unicode text that forms the version
 * @param tlen the length in UChars
 * @param log the log to record errors in
 * @return 1 if it worked
 */ 
static int add_first_version( MVD *mvd, adder *add, UChar *text, 
    int tlen, plugin_log *log )
{
    int res = 1;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, 1 );
        pair *p = pair_create_basic( bs, text, tlen );
        res = mvd_add_pair( mvd, p );
        if ( !res )
            plugin_log_add(log,
            "mvd_add: failed to add first version\n");
        else
        {
            version *v = version_create( adder_vid(add), 
                adder_version_desc(add) );
            if ( v != NULL )
                res = mvd_add_version( mvd, v );
            if ( res == 0 )
                plugin_log_add(log,
                "mvd_add: failed to add version\n");
        }
    }
    return res;
}
/**
 * Wrap an array of pairs into a doubly linked list
 * @param pairs a pairs list as a dyn_array
 * @param log the log to add it to
 * @param o the orphanage to register transpositions in
 * @return head of the cards list or NULL
 */
static card *link_pairs( dyn_array *pairs, plugin_log *log, orphanage *o )
{
    int len = dyn_array_size( pairs );
    int i;
    card *head = NULL;
    card *current;
    for ( i=0;i<len;i++ )
    {
        pair *p = (pair*)dyn_array_get( pairs, i );
        card *c = card_create( p, log );
        if ( c != NULL )
        {
            if ( head == NULL )
                current = head = c;
            else
            {
                card_set_right( current, c );
                card_set_left( c, current );
                current = c;
            }
            if ( pair_is_parent(p) )
                orphanage_add_parent(o,c);
            else if ( pair_is_child(p) )
                orphanage_add_child(o,c);
        }
        else
        {
            if ( head != NULL )
                card_dispose_list( head );
            head = NULL;
            break;
        }
    }
    return head;
}
/**
 * Add a new version to the MVD
 * @param add the configured add setup
 * @param mvd the MVD to add it to
 * @return 1 if it worked, else 0
 */
static int add_version( adder *add, MVD *mvd )
{
    int res = 0;
    version *new_v = version_create( adder_vid(add), adder_version_desc(add) );
    if ( new_v != NULL )
        res = mvd_add_version( mvd, new_v );
    return res;
}
/**
 * Print the unmerged segments held in the deviants array
 * @param deviants a simple array of segments in-between aligned ones
 */
static void print_unmerged_segments( dyn_array *deviants )
{
    int i;
    printf("unmerged segments:\n");
    for ( i=0;i<dyn_array_size(deviants);i++ )
    {
        card *e = dyn_array_get( deviants, i );
        if ( pair_is_child(card_pair(e)) )
            printf("child: %d-%d\n",card_text_off(e),card_text_off(e)+card_len(e) );
        else if ( card_len(e)==0 )
            printf("empty: %d-%d\n",card_text_off(e),card_text_off(e)+card_len(e));
        else
            printf("mismatch: %d-%d\n",card_text_off(e),card_text_off(e)+card_len(e));
    }
}
/**
 * Print the merged segments only
 * @param c the first card in the list
 * @param nv a bitset containing only the new version
 * @param size the size of version-sets
 */
static int print_merged_segments( card *c, bitset *nv, int size )
{
    int res = 1;
    card *e = c;
    printf("merged segments:\n");
    int last_off = 0;
    while ( e != NULL )
    {
        char buf[16];
        const char *parent;
        bitset_tostring(pair_versions(card_pair(e)),buf,size+2);
        int end = card_len(e)+card_text_off(e);
        parent = (pair_is_parent(card_pair(e)))?"[p]":"";
        printf("[%s] %d-%d %s\n",buf,card_text_off(e),end,parent);
        if ( card_text_off(e) < last_off )
        {
            printf("alignment overlaps: %d followed by %d not allowed\n",
                last_off,card_text_off(e));
            c = NULL;
            res = 0;
            break;
        }
        last_off = end;
        e = card_next(e, nv, 0);
    }
    return res;
}
static int path_v_exists( card *l, card *r, int v )
{
    if ( card_node_to_right(l) && card_node_to_left(r) && card_right(l)!=r )
        return 0;
    else
        return 1;
}
/**
 * Can we go from l to r on some path NOT involving the new version?
 * @param l the left limit of the path, left of r
 * @param r the right limit o the path, right of l
 * @param version the new version 
 * @param concrete if 1 then the path must contain at least one character
 * @return 1 if it has such a path else 0
 */
static int path_exists( card *l, card *r, int version, int concrete )
{
    if ( l == r || card_right(l) == r )
        return 0;
/*
    else if ( card_node_to_right(l) && card_node_to_left(r) )
    {
        printf("inserting empty arc for deletion in version %d:\n", version);
        card_print_until( l, r );
        return 1;
    }
 */
    else
    {
        bitset *lv = pair_versions(card_pair(l));
        bitset *rest = bitset_clone( lv );
        if ( rest != NULL )
        {
            bitset *rv = pair_versions(card_pair(r));
            bitset_and( rest, rv );
            bitset_clear_bit( rest, version );
            if ( !bitset_empty(rest) )
            {
                card *c = card_right(l);
                while ( c != r )
                {
                   pair *p = card_pair( c );
                   bitset *bs = pair_versions( p );
                   if ( (!concrete || pair_len(p) > 0) 
                       && bitset_intersects(bs,rest) )
                   {
/*
                       printf("inserting empty arc for deletion in version %d:\n",
                           version);
                       card_print_until( l, r );
*/
                       return 1;
                   }
                   c = card_right(c);
                }
           }
           bitset_dispose( rest );
        }
        return 0;
    }
}
/**
 * Check that there are no gaps in the sequence of aligned and deviant sections
 * @param list the list of cards
 * @param nv a set of versions containing the new version only
 * @param deviants the mismatch and child transpose segments
 */
static int check_version_integrity( card *list, bitset *nv, dyn_array *deviants )
{
    int res = 1;
    int pos = 0;
    int j = 0;
    card *c = list;
    card *d = dyn_array_get(deviants,j++);
    while ( c != NULL )
    {
        while ( d!=NULL && card_text_off(d)==pos )
        {
            pos = card_end(d);
            if ( j<dyn_array_size(deviants) )
                d = dyn_array_get(deviants,j++);
            else
                d = NULL;
        }
        pair *p = card_pair(c);
        bitset *cv = pair_versions(p);
        if ( pos != card_text_off(c) )
        {
            res = 0;
            printf("gap in card list\n");
        }
        pos = card_end(c);
        c = card_next( c, nv, 0 );
    }
    return res;
}
static int card_dangles( card *c, card *sentinel )
{
    int res = 1;
    card *temp = card_right(c);
    pair *cp = card_pair(c);
    bitset *cpv = pair_versions( cp );
    while ( temp != sentinel )
    {
        pair *p = card_pair(temp);
        bitset *bs = pair_versions(p);
        if ( bitset_intersects(cpv,bs) )
        {
            res = 0;
            break;
        }
        temp = card_right(temp);
    }
    return res;
}
/**
 * Sort the discards by their start offsets in the new version, 
 * then insert them in order into the list. Fill in any gaps with empty cards.
 * @param list the list of card comprising the current alignment
 * @param deviants array of deviants (discards+children)
 * @param version the version we are comparing with
 * @param log the log to record errors in
 * @return 1 if it worked, else 0
 */
static int add_deviant_pairs( card *list, dyn_array *deviants, int version, 
    plugin_log *log )
{
    int i,res = 1;
    int j = 0;
    bitset *nv = bitset_create();
    bitset_set(nv,version);
    int pos = 0;
    dyn_array_sort( deviants, card_compare );
    card *sentinel = NULL;
    card *c = card_first(list,nv);
    card *d = dyn_array_get( deviants, 0 );// NB: check if array not empty
    card *old_c = NULL;
    // debug
    res = print_merged_segments( c, nv, version );
    if ( res )
    {
        print_unmerged_segments( deviants );
        res = check_version_integrity( list, nv, deviants );
    }
    // end debug
    while ( res && c != NULL )
    {
        //printf("pos: %d c: %d-%d",pos,card_text_off(c),card_len(c)
        //    +card_text_off(c));
        // test for variants, transpositions and empty arcs
        if ( d != NULL && card_text_off(d)==pos )
        {
            if ( old_c == NULL )
                card_add_before( c, d );
            else
            {
                int insertion = (card_len(d)!=0 
                    && !path_exists(old_c,c,version,0));
                card *ip = card_get_insertion_point_nv( old_c, c, version );
                if ( ip != NULL )
                    card_add_after(ip,d);
                else
                    plugin_log_add(log,
                       "failed to obtain insertion point");
                // test for insertion in new version
                if ( insertion )
                {
                    bitset *bs = bitset_create();
                    if ( bs != NULL )
                    {
                        bitset_or( bs, pair_versions(card_pair(old_c)) );
                        bitset_and( bs, pair_versions(card_pair(c)) );
                        bitset_clear_bit( bs, version );
                        if ( !bitset_empty(bs) )
                        {
                            card *blank = card_create_blank_bs( bs, log );
                            card_add_after( d, blank );
/*
                            card *ip = card_get_insertion_point_other( old_c, 
                                c, blank ); 
                            if ( ip != NULL )
                                card_add_after( ip, blank );
                            else
                                plugin_log_add(log,
                                    "failed to obtain insertion point");
*/ 
                        }
                        bitset_dispose( bs );
                    }
                    else
                        plugin_log_add(log,"mvd_add: failed to create bs");
                }
            }
            pos = card_end(d);
            d = dyn_array_get(deviants,++j);
        }
        // test for deletion in the new version
        else if ( old_c != NULL && card_text_off(c) == pos 
            /*&& c != sentinel */&& !path_v_exists(old_c,c,version) )
        {
            bitset *bs = bitset_create();
            if ( bs != NULL )
            {
                bitset_set( bs, version );
                card *blank = card_create_blank_bs( bs, log );
                if( blank != NULL )
                {
                    card_set_text_off( blank, pos );
                    card *ip = card_get_insertion_point_nv( old_c, c, version );
                    if ( ip != NULL )
                        card_add_after( ip, blank );
                    else
                        plugin_log_add(log,
                       "failed to obtain insertion point");
                }
                bitset_dispose( bs );
            }
        }
        // move to next card if there are no more d's at this pos
        if ( d==NULL || card_text_off(d) != pos )
        {
            if ( d!= NULL && card_text_off(d) < pos )
                printf("failed to insert mismatch at %d, length %d\n",
                    pos,card_len(d));
            pos = card_end( c );
            old_c = c;
            c = card_next( c, nv, 0 );
        }
        // handle end of list case
        if ( c == NULL )
        {
            if ( old_c != NULL )
            {
                if ( old_c != sentinel )
                {
                    bitset *bs = bitset_create();
                    if ( bs != NULL )
                    {
                        for  ( i=1;i<=version;i++ )
                            bitset_set( bs, i );
                        sentinel = c = card_create_blank_bs( bs, log );
                        card_set_text_off( c, card_end(old_c) );
                        if ( sentinel != NULL )
                            card_append( old_c, sentinel );
                        bitset_dispose( bs );
                        c = sentinel;
                    }
                }
                else 
                {
                    card *left = card_left( sentinel );
                    pair *b = card_pair(sentinel);
                    // check for unfinished nodes
                    bitset *bs = bitset_clone(pair_versions(b));
                    bitset *dangling = bitset_create();
                    card *temp = left;
                    while ( !bitset_empty(bs) )
                    {
                        pair *q = card_pair( temp );
                        if ( card_dangles(temp,sentinel) )
                        {
                            bitset_or(dangling,pair_versions(q) );
                        }
                        bitset_and_not( bs, pair_versions(q) );
                        temp = card_left(temp);
                    }
                    bitset *final = pair_versions(b);
                    bitset_and_not( final, dangling );
                    if ( !bitset_empty(final) )
                    {
                        card *blank = card_create_blank_bs( final,log );
                        card_add_after( left, blank );
                    }
                    card_remove( sentinel, 1 );
                    pair_dispose( b );
                    bitset_dispose(bs);
                    bitset_dispose( dangling );
                    break;
                }
            }
        }
    }
    return res;
}
/**
 * Add alignment to rejects collection for later insertion
 * @param a the alignment to add
 * @param discards the discards pile
 * @param log the log to record errors in
 */
static void alignment_add_to_rejects( alignment *a, dyn_array *discards,
    plugin_log *log )
{
    card *reject = alignment_to_card(a,log);
    dyn_array_add( discards, reject );
}
/**
 * Get a version's short name in 8-bit chars
 * @param mvd the mvd
 * @param buf a buffer to hold the name
 * @param length of buf
 * @param vid the version id
 */
static void get_mvd_short_name( MVD *mvd, char *buf, int len, int vid )
{
    int nversions = mvd_count_versions(mvd);
    version **versions = calloc( nversions, sizeof(version*) );
    if ( versions != NULL )
    {
        int res = mvd_get_versions( mvd, versions, nversions );
        if ( res )
        {
            u_print( version_id(versions[vid-1]), buf, len );
        }
        free( versions );
    }
}
/**
 * Debug routine
 */
int list_verify( card *list, int version )
{
    int res = 1;
    bitset *bs = bitset_create();
    card *c = list;
    int pos = 0;
    hashmap *hm = hashmap_create( 200, 1);
    while (c!=NULL)
    {
        UChar buf[16];
/*
        if ( !card_exists(c) )
            printf("Duff card!\n");
*/
        calc_ukey( buf, (long)c, 16 );
        if ( hashmap_contains(hm,buf) )
            printf("list error: loop in list\n");
        else
            hashmap_put( hm, buf, c );
        if ( bitset_next_set_bit(pair_versions(card_pair(c)),version)==version )
        {
            if ( card_text_off(c) < pos )
            {
                printf("list error, card offset %d less than previous end %d\n",
                    card_text_off(c),pos );
            }
            pos = pair_len(card_pair(c))+card_text_off(c);
        }
        if ( card_left(c)!= NULL )
        {
            calc_ukey( buf, (long)card_left(c), 16 );
            if ( !hashmap_contains(hm,buf) )
                printf("list error: left pointer not already seen\n");
        }
        card *old_c = c;
        c = card_right( c );
        if ( c != NULL )
        {
            card *d = card_left( c );
            if ( d != old_c )
                printf("list: back link broken\n");
        }
    }
    bitset_dispose(bs);
    hashmap_dispose( hm, NULL );
    return res;
}
/**
 * Add a version to an MVD that already has at least one
 * @param mvd the mvd to add it to
 * @param add the mvd_add object containing options
 * @param text the text to add
 * @param tlen the length of text in UChars
 * @param log the log to record errors in
 * @return 1 if it worked
 */
static int add_subsequent_version( MVD *mvd, adder *add, 
    UChar *text, int tlen, plugin_log *log )
{
    int res = 1;
    orphanage *o = orphanage_create();
    if ( o != NULL )
    {
        dyn_array *discards = dyn_array_create( 20 );
        if ( discards != NULL )
        {
            int new_vid = mvd_count_versions(mvd)+1;
            bitset *nv = bitset_create();
            bitset_set(nv,new_vid);
            dyn_array *pairs = mvd_get_pairs( mvd );
            card *list = link_pairs( pairs, log, o );
            //list_verify(list, new_vid);
            // now add its version to the mvd
            if ( list != NULL )
            {
                // create initial alignment
                alignment *a = alignment_create( text, 0, tlen, tlen,
                    new_vid, o, log, list );
                if ( a != NULL )
                {
                    // maintain list of current alignments
                    alignment *head = a;
                    res = add_version( add, mvd );
                    // iterate while there are still alignments to do
                    while ( res && head != NULL )
                    {
                        alignment *left,*right;
                        //printf("aligning %d-%d\n",
                        //    alignment_start(head),alignment_end(head));
                        res = alignment_align( head, list );
                        if ( res )
                        {
                            //mum_verify(alignment_mum(head),new_vid);
                            res = alignment_merge( head, &left, &right, discards );
                            //mum_verify(alignment_mum(head),new_vid);
                            list_verify(list, new_vid);
                        }
                        alignment *old = head;
                        head = alignment_pop( head );
                        // if merged and aligned
                        if ( res )
                        {
                            if ( left != NULL )
                            {
                               // printf("pushing alignment %d-%d\n",
                                //    alignment_start(left),alignment_end(left));
                                head = alignment_push( head, left );
                            }
                            if ( right != NULL )
                            {
                                //printf("pushing alignment %d-%d\n",
                               //     alignment_start(right),alignment_end(right));
                                head = alignment_push( head, right );
                            }
                        }
                        else
                        {
                           // printf("rejecting alignment %d-%d\n",
                           //     alignment_start(old),alignment_end(old));
                            alignment_add_to_rejects( old, discards, log );
                            res = 1;
                        }
                        alignment_dispose( old );
                    }
                    char buf[16];
                    get_mvd_short_name(mvd,buf,16,new_vid);
                    //printf("mvd_add: aligned version %s\n",buf);
                    // add children to discards
                    int i,success,num;
                    card **children = orphanage_all_new_children(o,new_vid,
                        &num,&success);
                    if ( !success )
                    {
                        res = 0;
                        plugin_log_add(log,"failed to gather children\n");
                    }
                    else if ( children != NULL )
                    {
                        for ( i=0;i<num;i++ )
                            dyn_array_add( discards, children[i] );
                    }
                    if ( !card_verify_gaps(list,new_vid) )
                    {
                        plugin_log_add( log,"gaps invalid");
                    }
                    res = add_deviant_pairs( list, discards, new_vid, log );
                    if ( res )
                    {
                        pairs = card_to_pairs( list );
                        mvd_set_pairs( mvd, pairs );
                        if ( !verify_check(pairs) )
                        {
                            plugin_log_add(log,"error: unbalanced graph\n"); 
                        }
                        card_print_list( list );
                    }
                    if ( children != NULL )
                        free( children );
                }
                card_dispose_list( list );
            }
            dyn_array_dispose( discards );
        }
        orphanage_dispose( o );
    }
    return res;
}
/**
 * Add the supplied text to the MVD
 * @param add the add struct initialised and ready to go
 * @param mvd the MVD to add it to
 * @return 1 if it worked else 0
 */
static int add_mvd_text( adder *add, MVD *mvd, 
    unsigned char *data, size_t data_len, plugin_log *log )
{
    int res = 1;
    int tlen;
    char *mvd_encoding = mvd_get_encoding(mvd);
    if ( adder_encoding(add) != NULL )
    {
        // set the mvd encoding to add->encoding if it is empty
        if ( mvd_count_versions(mvd)==0 )
            mvd_set_encoding(mvd,adder_encoding(add));
        else if ( strcmp(mvd_encoding,adder_encoding(add))!=0 )
        {
            // just a warning, we'll continue
            plugin_log_add(log,
                "file's encoding %s does not match mvd's (%s):"
                " assimilating...\n",adder_encoding(add),mvd_encoding);
        }
    }
    // convert from the external add->encoding to UTF-16 
    UChar *text = convert_to_unicode( data, data_len, 
        adder_encoding(add), &tlen, log );
    if ( tlen > 0 )
    {
        if ( mvd_count_versions(mvd)==0 )
        {
            mvd_set_description( mvd, adder_mvd_desc(add) );
            res = add_first_version( mvd, add, text, tlen, log );
        }
        else
            res = add_subsequent_version( mvd, add, text, tlen, log );
        free( text );
    }
    else
    {
        plugin_log_add(log,"mvd_add: text is empty\n");
        res = 0;
    }
    return res;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulate or read
 * @param options a string containing the plugin's options
 * @param output buffer for errors, caller to dispose
 * @param data data passed directly in 
 * @param data_len length of data
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, char **output, unsigned char *data, 
    size_t data_len )
{
    int res = 1;
    *output = NULL;
    if ( *mvd == NULL )
        *mvd = mvd_create( 1 );
    if ( *mvd != NULL )
    {
        plugin_log *log = plugin_log_create();
        if ( log != NULL )
        {
            if ( *output != NULL )
                plugin_log_add(log,*output);
            if ( !mvd_is_clean(*mvd) )
                res = mvd_clean( *mvd );
            if ( res )
            {
                adder *add = adder_create(log);
                if ( add != NULL )
                {
                    hashmap *map = parse_options( options );
                    if ( map != NULL )
                    {
                        res = adder_set_options( add, map );
                        if ( res && data != NULL && data_len > 0 )
                            res = add_mvd_text( add, *mvd, data, data_len, 
                                log );
                        else
                            plugin_log_add(log,"mvd_add: length was 0\n");
                        hashmap_dispose( map, free );
                    }
                    else
                        plugin_log_add(log,"mvd_add: invalid options %s",
                            options);
                    adder_dispose( add );
                }
                else
                    plugin_log_add(log,
                        "mvd_add: failed to create mvd_add object\n");
            }
            else
            {
                plugin_log_add(log,"mvd failed to clean\n");
            }
        }
        else
        {
            plugin_log_add(log,"mvd_add: failed to create log object\n");
            res = 0;
        }
        *output = strdup( plugin_log_buffer(log) );
        plugin_log_dispose( log );
    }
    return res;
}
/**
 * Print a help message to stdout explaining what the parameters are
 */
char *help()
{
    return "options:\n\t"
        "text=<file>: file to add to existing MVD (optional)\n\t"
        "vid=<vid>: version ID as a /-delimited path\n\t"
        "encoding=<ascii-encoding-name>: the text encoding\n\t"
        "description=<string>: description of new version in encoding\n\t"
        "debug=1|0: turn debugging on (default off)\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    return "version 1.0 (c) 2014 Desmond Schmidt\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "add";
}
/**
 * Do we change the MVD?
 * @return 1 if we do else 0
 */
int changes()
{
    return 1;
}
/**
 * Provide a description of this plugin
 * @return a string
 */
char *description()
{
    return "adds a new version to an existing mvd\n";
}
int test( int *passed, int *failed )
{
#ifdef MVD_TEST
    return test_mvd_add( passed, failed );
#else
    return 1;
#endif
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
#ifdef MVD_TEST
static char *folder;

static char *create_path( char *dir, char *file )
{
    int len1 = strlen( dir );
    int len2 = strlen( file );
    char *path = malloc( len1+len2+2 );
    if ( path != NULL )
    {
        strcpy( path, dir );
        strcat( path, "/" );
        strcat( path, file );
    }
    return path;
}
static void bare_file_name( char *f_name, int n, char *full_name )
{
    char *pos = strrchr( full_name, '.' );
    int i,len = strlen( full_name );
    if ( pos != NULL )
        len -= strlen(pos);
    int limit = n-1;
    for ( i=0;i<len&&i<limit;i++ )
    {
        f_name[i] = full_name[i];
    }
    f_name[i] = 0;
}
/**
 * Read a directory
 * @return number of files found or 0 on failure
 */
static int read_dir( char *folder )
{
    int n_files = 0;
    DIR *dir;
    int res = 1;
    struct dirent *ent;
    UChar desc[6];
    char *output=NULL;
    MVD *mvd=mvd_create(1);
    mvd_set_encoding(mvd, "utf-8");
    ascii_to_uchar( "test", desc, 6 );
    mvd_set_description( mvd, desc);
    if ((dir = opendir(folder)) != NULL) 
    {
        while ((ent = readdir(dir)) != NULL && res) 
        {
            int flen;
            if ( strcmp(ent->d_name,".")!=0&&strcmp(ent->d_name,"..")!=0 )
            {
                char *path = create_path(folder,ent->d_name);
                if ( path != NULL )
                {
                    char *txt = read_file( path, &flen );
                    if ( txt == NULL )
                        break;
                    else
                    {
                        char options[128];
                        char f_name[128];
                        options[0] = 0;
                        bare_file_name( f_name, 128, ent->d_name );
                        printf("processing file %s\n",f_name);
                        strcat( options, "vid=" );
                        strcat( options, f_name );
                        strcat( options, " encoding=utf-8" );
                        strcat( options, " description=test" );
                        res = process( &mvd, options, &output, txt, flen );
                        n_files++;
                        if ( output != NULL )
                        {
                            printf("%s",output);
                            free( output );
                            output = NULL;
                        }
                        if ( !res )
                            printf("adding of file %s failed\n",f_name);
                        free( txt );
                    }
                    free( path );
                }
            }
        }
        closedir( dir );
    }
    else
        fprintf(stderr,"test: failed to open directory %s\n",folder);
    write_mvd( mvd, "test.mvd" );
    return n_files;
}
// arguments: folder of text files
int test_mvd_add( int *passed, int *failed )
{
    int64_t start = epoch_time();
    int res = read_dir( "tagore" );
    //int res = read_dir( "tests" );
    if ( res )
    {
        (*passed)++;
    }
    else
    {
        (*failed)++;
        fprintf(stderr,"mvd_add: failed to load test directory\n");
        res = 0;
    }
    int64_t end = epoch_time();
    printf("time taken=%ld\n",(end-start));
    return res;
}
#endif