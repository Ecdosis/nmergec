/* 
 * File:   orphanage.h
 * Author: desmond
 *
 * Created on September 18, 2013, 2:33 PM
 */

#ifndef ORPHANAGE_H
#define	ORPHANAGE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct orphanage_struct orphanage;
orphanage *orphanage_create();
void orphanage_dispose();
int orphanage_next_id( orphanage *o );
int orphanage_add_parent( orphanage *o, card *parent );
int orphanage_add_child( orphanage *o, card *child );
card *orphanage_get_parent( orphanage *o, card *child );
void orphanage_get_children( orphanage *o, card *parent, 
    card **children, int size );
int orphanage_count_children( orphanage *o, card *parent );
int orphanage_remove_parent( orphanage *o, card *parent );
#ifdef MVD_TEST
void orphanage_test( int *passed, int *failed );
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* ORPHANAGE_H */

