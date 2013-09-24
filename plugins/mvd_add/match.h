/* 
 * File:   match.h
 * Author: desmond
 *
 * Created on March 2, 2013, 4:45 PM
 */

#ifndef MATCH_H
#define	MATCH_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct match_struct match;
match *match_create( linkpair *start, int j, linkpair *pairs, suffixtree *st, 
    plugin_log *log );
match *match_copy( match *m, plugin_log *log );
match *match_clone( match *mt, plugin_log *log );
int match_restart( match *m );
void match_dispose( match *m );
void match_append( match *m1, match *m2 );
int match_advance( match *m, pos *loc, plugin_log *log );
bitset *match_versions( match *m );
match *match_extend( match *mt, UChar *text, 
    plugin_log *log );
int match_follows( match *first, match *second );
int match_prev_pos( match *m );
int match_st_off( match *m );
int match_inc_end_pos( match *m );
suffixtree *match_suffixtree( match *m );
int match_single( match *m, UChar *text, plugin_log *log );
int match_pop( match *m );
void match_set_versions( match *m, bitset *bs );
linkpair *match_start_link( match *m );
int match_start_pos( match *m );
linkpair *match_end_link( match *m );
int match_end_pos( match *m );
int match_len( match *m );
int match_total_len( match *m );
int match_compare( void *a, void *b );
int match_transposed( match *m, int new_version, int tlen );
int is_maximal( match *m, UChar *text );
void match_inc_freq( match *m );
int match_freq( match *m );
match *match_next( match *m );
void match_print( match *m, UChar *text );
int match_st_end( match *m );
int match_split( match *m, UChar *text, int v, plugin_log *log );

#ifdef	__cplusplus
}
#endif

#endif	/* MATCH_H */

