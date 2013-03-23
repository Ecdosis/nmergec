#include <stdlib.h>
#include <stdio.h>
#include "pos_item.h"


/**
 * Maintain a sorted queue of positions where we have already matched 
 * against the suffix tree. 
 */
struct pos_item_struct
{
    int start_p;
    int start_pos;
    pos_item *next;
};
/**
 * Create a position queue
 * @param start_p index into the pairs array
 * @param start_pos position along a given pair
 * @return the pos item
 */
pos_item *pos_item_create( int start_p, int start_pos )
{
    pos_item *pi = calloc( 1, sizeof(pos_item) );
    if ( pi == NULL )
        fprintf(stderr,"pos_item: failed to allocate\n");
    else
    {
        pi->start_p = start_p;
        pi->start_pos = start_pos;
    }
    return pi;
}
/**
 * Dispose of a pos_item
 * @param pi the item possibly forming a list
 */
void pos_item_dispose( pos_item *pi )
{
    if ( pi->next != NULL )
        pos_item_dispose( pi->next );
    free( pi );
}
/**
 * Compare two items a la strcmp
 * @param a the first item
 * @param b the second
 * @return 1 if a > b, -1 if b > a else 0
 */
static int item_compare( pos_item *a, pos_item *b )
{
    if ( a->start_p > b->start_p )
        return 1;
    else if ( a->start_p < b->start_p )
        return -1;
    else
    {
        if ( a->start_pos > b->start_pos )
            return 1;
        else if ( a->start_pos < b->start_pos )
            return -1;
        else
            return 0;
    }
}
/**
 * Add a position to the queue. Keep it sorted
 * @param pi the item list in question
 * @pram start_p the index into the pairs array
 * @param start_pos the index into the data of the pair
 * @return the list head or NULL
 */
pos_item *pos_item_add( pos_item *pi, int start_p, int start_pos )
{
    pos_item *head = pi;
    pos_item *it = pos_item_create( start_p, start_pos );
    if ( it != NULL )
    {
        pos_item *prev = pi;
        while ( pi != NULL && item_compare(it,pi)>0 )
        {
            prev = pi;
            pi = pi->next;
        }
        // pi is the element before which to insert
        if ( prev != pi )
            prev->next = it;
        else
            head = it;
        it->next = pi;
        return head;
    }
    else
    {
        fprintf(stderr,"pos_item: failed to add mnew item\n");
        return NULL;
    }
}
/**
 * Quickly examine the queue head
 * @param pi the pos_item in question
 * @param start_p the index into the pairs array
 * @param pos the data offset within that pair
 * @return 1 if the queried parameters match the top of the queue
 */
int pos_item_peek( pos_item *pi, int start_p, int start_pos )
{
    return pi->start_p==start_p&&pi->start_pos==start_pos;
}
/**
 * Pop the topmost element off the queue
 * @param pi the pos_item
 * @return the new queue head
 */
pos_item *pos_item_pop( pos_item *pi )
{
    pos_item *ret = pi->next;
    if ( pi != NULL )
        free( pi );
    return ret;
}
/**
 * Print the queue to stdout for debugging in queue order
 * @param pi the queue to print
 */
void pos_item_print( pos_item *pi )
{
    while ( pi != NULL )
    {
        printf("p_index: %d p_pos: %d\n",pi->start_p, pi->start_pos);
        pi = pi->next;
    }
}
/**
 * Test this object
 * @param passed VAR param increase by the number of passed tests
 * @param failed VAR param: increase by the number of failed tests
 */
void pos_item_test( int *passed, int *failed )
{
    int i;
    pos_item *pq = pos_item_create( 10, 5 );
    if ( pq == NULL )
        *failed += 1;
    else
    {
        *passed += 1;
        pq = pos_item_add( pq, 5, 15 );
        pq = pos_item_add( pq, 1, 95 );
        pq = pos_item_add( pq, 4, 45 );
        pq = pos_item_add( pq, 1, 15 );
        pq = pos_item_add( pq, 1, 23 );
        //pos_item_print(pq );
        pos_item *temp = pq;
        while ( temp != NULL )
        {
            if ( temp->next != NULL && item_compare(temp,temp->next)>0 )
            {
                fprintf(stderr,"pos_item: queue is out of order!\n");
                *failed += 1;
                break;
            }
            temp = temp->next;
        }
        if ( temp == NULL )
            *passed += 1;
        //printf("about to peek\n");
        if ( !pos_item_peek(pq,1,15) )
        {
            fprintf(stderr,"pos_item: peek failed\n");
            *failed += 1;
        }
        else
            *passed += 1;
        //printf("about to pop\n");
        for ( i=0;i<6;i++ )
            pq = pos_item_pop( pq );
        if ( pq != NULL )
        {
            *failed += 1;
            fprintf(stderr,"pos_item: pop failed\n");
        }
        else
            *passed += 1;
    }
}

#ifdef POS_ITEM_DEBUG
int main( int argc, char **argv )
{
    int passed = 0;
    int failed = 0;
    pos_item_test( &passed, &failed );
    printf( "passed %d tests failed %d\n",passed, failed);
}
#endif