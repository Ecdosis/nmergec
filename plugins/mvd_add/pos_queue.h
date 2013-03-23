/* 
 * File:   pos_queue.h
 * Author: desmond
 *
 * Created on March 13, 2013, 3:23 PM
 */

#ifndef POS_QUEUE_H
#define	POS_QUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct pos_queue_struct pos_queue;
pos_queue *pos_queue_create( plugin_log *log );
void pos_queue_dispose( pos_queue *pq );
int pos_queue_add( pos_queue *pq, int start_p, int start_pos );
int pos_queue_peek( pos_queue *pq, int start_p, int start_pos );
int pos_queue_pop( pos_queue *pq );
void pos_queue_print( pos_queue *pq );
void pos_queue_test( int *passed, int *failed );

#ifdef	__cplusplus
}
#endif

#endif	/* POS_QUEUE_H */

