/* 
 * File:   state.h
 * Author: desmond
 *
 * Created on March 2, 2013, 4:43 PM
 */

#ifndef STATE_H
#define	STATE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct state_struct state;
void state_append( state *s, state *t );
state *state_next( state *s );
bitset *state_versions( state *s );
state *state_split(state *s, bitset *bs );
void state_update( state *s, UChar c );
void state_merge( state *s, state *t );

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_H */

