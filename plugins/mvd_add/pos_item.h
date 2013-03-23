/* 
 * File:   pos_item.h
 * Author: desmond
 *
 * Created on March 13, 2013, 3:23 PM
 */

#ifndef POS_ITEM_H
#define	POS_ITEM_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct pos_item_struct pos_item;
pos_item *pos_item_create( int start_p, int start_pos);
void pos_item_dispose( pos_item *pi );
pos_item *pos_item_add( pos_item *pq, int start_p, int start_pos );
int pos_item_peek( pos_item *pq, int start_p, int start_pos );
pos_item *pos_item_pop( pos_item *pi );
int pos_item_empty( pos_item *pq );
void pos_item_print( pos_item *pq );
void pos_item_test( int *passed, int *failed );

#ifdef	__cplusplus
}
#endif

#endif	/* POS_QUEUE_H */

