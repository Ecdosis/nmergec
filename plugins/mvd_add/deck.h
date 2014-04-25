/* 
 * File:   deck.h
 * Author: desmond
 *
 * Created on March 2, 2013, 7:52 AM
 */

#ifndef DECK_H
#define	DECK_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct deck_struct deck;
deck *deck_create( alignment *a, card *cards );
void deck_dispose( deck *m );
int deck_align( deck *m );
match *deck_get_mum( deck *m );
int deck_mum_ok( deck *mt, match *mum );


#ifdef	__cplusplus
}
#endif

#endif	/* DECK_H */

