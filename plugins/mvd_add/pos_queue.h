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


#ifdef	__cplusplus
}
#endif

#endif	/* POS_QUEUE_H */

