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
match *match_create( int i, int j, pair **pairs, suffixtree *st, int end, 
        plugin_log *log );
match *match_clone( match *mt, plugin_log *log );
int match_restart( match *m );
void match_dispose( match *m );
void match_append( match *m1, match *m2 );
int match_advance( match *mt );
match *match_extend( match *mt, pos_item **avoids, UChar *text, 
    plugin_log *log );
int match_follows( match *first, match *second );
int match_prev_pos( match *m );
int match_inc_end_pos( match *m );
suffixtree *match_suffixtree( match *m );
int match_single( match *m, UChar *text );
void match_set_versions( match *m, bitset *bs );
int match_start_index( match *m );
int match_start_pos( match *m );
int match_end_index( match *m );
int match_end_pos( match *m );
int match_len( match *m );
int match_total_len( match *m );
int match_compare( void *a, void *b );
int is_maximal( match *m, UChar *text );
void match_inc_freq( match *m );
int match_freq( match *m );
void match_print( match *m, UChar *text );

#ifdef	__cplusplus
}
#endif

#endif	/* MATCH_H */

