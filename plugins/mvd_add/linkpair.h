/* 
 * File:   linkpair.h
 * Author: desmond
 *
 * Created on March 21, 2013, 6:20 AM
 */

#ifndef LINKPAIR_H
#define	LINKPAIR_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct linkpair_struct linkpair;
linkpair *linkpair_create( pair *p, plugin_log *log );
void linkpair_add_hint( linkpair *lp, int version, plugin_log *log );
linkpair *linkpair_make_parent( linkpair *p, plugin_log *log );
linkpair *linkpair_make_child( linkpair *parent, int version, plugin_log *log );
void linkpair_replace( linkpair *old_lp, linkpair *new_lp );
void linkpair_dispose( linkpair *lp );
void linkpair_dispose_list( linkpair *lp );
void linkpair_set_left( linkpair *lp, linkpair *left );
void linkpair_set_right( linkpair *lp, linkpair *right );
linkpair *linkpair_left( linkpair *lp );
linkpair *linkpair_right( linkpair *lp );
linkpair *linkpair_next( linkpair *lp, bitset *bs );
void linkpair_set_st_off( linkpair *lp, int st_off );
int linkpair_st_off( linkpair *lp );
pair *linkpair_pair( linkpair *lp );
dyn_array *linkpair_to_pairs( linkpair *lp );
int linkpair_node_to_right( linkpair *lp );
int linkpair_node_to_left( linkpair *lp );
bitset *linkpair_node_overhang( linkpair *lp );
int linkpair_add_at_node( linkpair *lp, linkpair *after, int verify );
int linkpair_add_after( linkpair *lp, linkpair *after );
int linkpair_list_circular( linkpair *lp );
void linkpair_remove( linkpair *lp, int dispose );
void linkpair_test( int *passed, int *failed );
void linkpair_print( linkpair *lp );
extern UChar USTR_EMPTY[];

#ifdef	__cplusplus
}
#endif

#endif	/* LINKPAIR_H */

