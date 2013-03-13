#include <stdlib.h>
#include "pos_queue.h"

#define MAX_QUEUE_LEN 32

/**
 * Maintain a sorted queue of positions where we have already matched 
 * against the suffix tree. 
 */
typedef struct item_struct item;
struct item_struct
{
    int start_p;
    int start_pos;
};
struct pos_queue_struct
{
    item *items;
    int size;
};

/**
 * Create a position queue
 * @param log the log to record errors in
 * @return the position queue
 */
pos_queue *pos_queue_create( plugin_log *log )
{
    pos_queue *pq = calloc( 1, sizeof(pos_queue) );
    if ( pq != NULL )
    {
        pq->items = calloc( MAX_QUEUE_LEN, sizeof(item) );
        if ( pq->items == NULL )
        {
            plugin_log_add(log,"pos_queue: failed to allocate queue\n");
            free( pq );
            pq = NULL;
        }
    }
    return pq;
}
/**
 * Dispose of a pos_queue
 * @param pq the queue in question
 */
void pos_queue_dispose( pos_queue *pq )
{
    if ( pq->items != NULL )
        free( pq->items );
    free( pq );
}
/**
 * Compare two items
 * @param a the first item
 * @param b the second item
 * @return 0 if equal, -1 if a<b else 1 if a>b
 */
static int item_compare( item *a, item *b )
{
    if ( a->start_p < b->start_p )
        return -1;
    else if ( a->start_p > b->start_p )
        return 1;
    else
    {
        if ( a->start_pos < b->start_pos )
            return -1;
        else if ( a->start_pos > b->start_pos )
            return 1;
        else
            return 0;
    }
}
/**
 * Resort the queue in decreasing order after an addition
 * @param pq the pos_queue
 */
static void pos_queue_sort( pos_queue *pq )
{
    int i,gap;
    for ( i=1;i<pq->size;i++ )
    {
        item it = pq->items[i];
        gap = i;
        while ( gap > 0 && item_compare(&it,&pq->items[gap-1])>0 )
        { 
            pq->items[gap] = pq->items[gap-1];
            gap--;
        }
        pq->items[gap] = it; 
    }
}
/**
 * Add a position to the queue. Keep it sorted
 * @param pq the queue in question
 * @pram start_p the index into the pairs array
 * @param start_pos the index into the data of the pair
 * @return 1 if it worked else 0
 */
int pos_queue_add( pos_queue *pq, int start_p, int start_pos )
{
    item it;
    it.start_p = start_p;
    it.start_pos = start_pos;
    if ( pq->size+1 < MAX_QUEUE_LEN )
    {
        pq->items[pq->size++] = it;
        pos_queue_sort( pq );
        return 1;
    }
    else
        return 0;
}
/**
 * Quickly examine the queue
 * @param pq the pos_queue in question
 * @param start_p the index into the pairs array
 * @param pos the data offset within that pair
 * @return 1 if the queried parameters match the top of the queue
 */
int pos_queue_peek( pos_queue *pq, int start_p, int start_pos )
{
    if ( pq->size > 0 )
    {
        item *it = &pq->items[pq->size-1];
        return it->start_p==start_p && it->start_pos==start_pos;
    }
    else
        return 0;
}
/**
 * Pop the topmost element off the queue
 * @param pq the pos_queue
 */
int pos_queue_pop( pos_queue *pq )
{
    if ( pq->size > 0 )
    {
        pq->size--;
        return 1;
    }
    else
        return 0;
}
