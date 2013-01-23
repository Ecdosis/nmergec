/* 
 * File:   char_buf.h
 * Author: desmond
 *
 * Created on January 14, 2013, 3:21 PM
 */

#ifndef CHAR_BUF_H
#define	CHAR_BUF_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct char_buf_struct char_buf;
char_buf *char_buf_create( int initial );
int char_buf_write( char_buf *cb, unsigned char *data, int len );
unsigned char *char_buf_get( char_buf *cb, int *len );
#ifdef MVD_TEST
int test_char_buf( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* CHAR_BUF_H */

