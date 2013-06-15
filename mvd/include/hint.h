/* 
 * File:   hint.h
 * Author: desmond
 *
 * Created on June 13, 2013, 10:54 AM
 */

#ifndef HINT_H
#define	HINT_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct hint_struct hint;

hint *hint_create( bitset *versions, vgnode *n );
void hint_dispose( hint *h );
hint *hint_contains( hint *h, bitset *bs );
int hint_subtract( hint *h, bitset *other );
hint *hint_delist( hint *h );
void hint_append( hint *h, hint *other );
vgnode *hint_node( hint *h );
void hint_or( hint *h, bitset *bs );

#ifdef	__cplusplus
}
#endif

#endif	/* HINT_H */

