#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "char_buf.h"
#define MIN_BLOCK_SIZE 1024
#define max(a,b) (a>b)?a:b
struct char_buf_struct
{
    unsigned char *buf;
    int allocated;
    int used;
    int initial;
};
/**
 * Create a char_buf: a dynamically resizeable character buffer
 * @param initial initial hint at how big
 * @return the allocated char buffer
 */
char_buf *char_buf_create( int initial )
{
    char_buf *cb = calloc( 1, sizeof(char_buf) );
    if ( cb != NULL )
    {
        cb->initial = max(initial,MIN_BLOCK_SIZE);
        cb->buf = malloc( cb->initial );
        if ( cb->buf == NULL )
        {
            fprintf(stderr,"char_buf: failed to allocate buffer\n");
            free( cb );
            cb = NULL;
        }
        else
            cb->allocated = cb->initial;
    }
    else
        fprintf(stderr,"char_buf: failed to allocate object\n");
    return cb;
}
/**
 * Dispose of this char buffer
 * @param cb the buffer in question
 */
void char_buf_dispose( char_buf *cb )
{
    if ( cb->buf != NULL )
        free( cb->buf );
    free( cb );
}
/**
 * Try to write to this buffer
 * @param cb the buffer in question
 * @param data the data to write
 * @param len the length of the data
 * @return 1 if successful
 */
int char_buf_write( char_buf *cb, unsigned char *data, int len )
{
    int res = 1;
    if ( cb->used+len >= cb->allocated )
    {
        int new_size = cb->used+len + cb->initial/2;
        unsigned char *new_buf = malloc( new_size );
        if ( new_buf != NULL )
        {
            memcpy( new_buf, cb->buf, cb->used );
            free( cb->buf );
            cb->buf = new_buf;
            cb->allocated = new_size;
        }
        else
        {
            fprintf(stderr,"char_buf: failed to reallocate buffer\n");
            return 0;
        }
    }
    memcpy( &cb->buf[cb->used], data, len );
    cb->used += len;
    return res;
}
/**
 * Get this char_buf's internal buffer
 * @param cb the char_buf inn question
 * @param VAR param len: used length of the fetched buffer
 */
unsigned char *char_buf_get( char_buf *cb, int *len )
{
    *len = cb->used;
    return cb->buf;
}