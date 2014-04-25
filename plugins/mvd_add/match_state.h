/* 
 * File:   match_state.h
 * Author: desmond
 *
 * Created on March 27, 2013, 8:02 AM
 */

#ifndef MATCH_STATE_H
#define	MATCH_STATE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct match_state_struct match_state;
match_state *match_state_create( location *start, location *end, 
    int text_off, int len, bitset *bs, pos *loc, plugin_log *log );
match_state *match_state_copy( match_state *ms, plugin_log *log );
void match_state_dispose( match_state *ms );
void match_state_push( match_state *head, match_state *ms );
void match_state_loc( match_state *ms, pos *loc );
match_state *match_state_next( match_state *ms );
location match_state_start( match_state *ms );
location match_state_end( match_state *ms );
int match_state_text_off( match_state *ms );
int match_state_len( match_state *ms );
bitset *match_state_bs( match_state *ms );


#ifdef	__cplusplus
}
#endif

#endif	/* MATCH_STATE_H */

