/* 
 * File:   deck.h
 * Author: desmond
 *
 * Created on 15 March 2013, 3:32 PM
 */

#ifndef DECK_H
#define	DECK_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct deck_struct deck;
deck *deck_create( UChar *text, int start, int len, int tlen, 
    int version, orphanage *o, plugin_log *log );
void deck_print( deck *a, const char *prompt );
void deck_dispose( deck *a );
UChar *deck_text( deck *a, int *tlen );
deck *deck_push( deck *head, deck *tail );
deck *deck_pop( deck *head );
int deck_version( deck *a );
plugin_log *deck_log( deck *a );
suffixtree *deck_suffixtree( deck *a );
int deck_len( deck *a );
int deck_tlen( deck *a );
int deck_align( deck *a, card **cards, 
    deck **left, deck **right, orphanage *o );
int deck_start( deck *a );
#ifdef	__cplusplus
}
#endif

#endif	/* DECK_H */
