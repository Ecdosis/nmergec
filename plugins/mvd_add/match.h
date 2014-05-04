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
#define MATCH_BASE \
    location start; \
    location end; \
    int text_off; \
    int len; \
    bitset *bs
    
typedef struct match_struct match;
match *match_create( card *start, int j, card *cards, suffixtree *st, 
    int st_off, plugin_log *log );
match *match_copy( match *m, plugin_log *log );
match *match_clone( match *mt, plugin_log *log );
int match_restart( match *m, int v, plugin_log *log );
void match_dispose( match *m );
void match_append( match *m1, match *m2 );
int match_advance( match *m, pos *loc, int v, plugin_log *log );
bitset *match_versions( match *m );
int match_update( match *m, card *cards );
int match_set( match *m, card *cards );
match *match_extend( match *mt, UChar *text, int v, plugin_log *log );
int match_follows( match *first, match *second );
int match_prev_pos( match *m );
int match_text_off( match *m );
int match_inc_end_pos( match *m );
suffixtree *match_suffixtree( match *m );
int match_single( match *m, UChar *text, int v, plugin_log *log, int popped );
int match_pop( match *m );
void match_set_versions( match *m, bitset *bs );
location match_start( match *m );
location match_end( match *m );
int match_len( match *m );
int match_total_len( match *m );
int match_compare( void *a, void *b );
int is_maximal( match *m, UChar *text );
void match_inc_freq( match *m );
int match_freq( match *m );
match *match_next( match *m );
void match_print( match *m, UChar *text );
int match_text_end( match *m );
int match_within_threshold( int distance, int length );
void match_verify_end( match *m );
void match_verify( match *m, UChar *text, int tlen );
#ifdef	__cplusplus
}
#endif

#endif	/* MATCH_H */

