/* 
 * File:   card.h
 * Author: desmond
 *
 * Created on March 21, 2013, 6:20 AM
 */

#ifndef CARD_H
#define	CARD_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct card_struct card;
card *card_create( pair *p, plugin_log *log );
card *card_create_blank( int version, plugin_log *log );
card *card_create_blank_bs( bitset *bs, plugin_log *log );
bitset *card_compute_extra_blank( card *c, card *after );
void card_add_hint( card *lp, int version, plugin_log *log );
card *card_make_parent( card **p, plugin_log *log );
card *card_make_child( card *parent, int version, plugin_log *log );
void card_replace( card *old_lp, card *new_lp );
void card_dispose( card *lp );
void card_dispose_list( card *lp );
void card_set_left( card *lp, card *left );
void card_set_right( card *lp, card *right );
card *card_get_insertion_point_nv( card *l, card *r, int v );
card *card_get_insertion_point_other( card *l, card *r, card *c );
card *card_left( card *lp );
card *card_right( card *lp );
card *card_next( card *lp, bitset *bs, int avoid );
card *card_first( card *list, bitset *bs );
card *card_next_nonempty( card *c, bitset *bs, int v );
bitset *card_overlap( card *c, bitset *bs );
card *card_prev( card *lp, bitset *bs );
void card_set_text_off( card *lp, int text_off );
int card_text_off( card *lp );
int card_end( card *c );
int card_len( card *lp );
int card_verify_gaps( card *list, int v );
pair *card_pair( card *lp );
dyn_array *card_to_pairs( card *lp );
int card_node_to_right( card *lp );
int card_node_to_left( card *lp );
bitset *card_node_overhang( card *lp );
int card_add_at_node( card *lp, card *after, int verify );
void card_add_after( card *lp, card *after );
void card_append( card *c, card *after );
int card_list_circular( card *lp );
void card_remove( card *lp, int dispose );
void card_test( int *passed, int *failed );
void card_print( card *lp );
void card_print_list( card *list );
void card_print_until( card *from, card *to );
void card_add_at( card **list, card *lp, int text_off, int version );
char *card_tostring( card *c );
int card_compare( void *c1, void *c2 );
card *card_merged_right( card *c, int version );
card *card_merged_left( card *c, int version );
card *card_prev_nonempty( card *c, bitset *bs );
//int card_exists( card *c );

extern UChar USTR_EMPTY[];

#ifdef	__cplusplus
}
#endif

#endif	/* CARD_H */

