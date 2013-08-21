/* 
 * File:   operation.h
 * Author: desmond
 *
 * Created on January 16, 2013, 8:48 AM
 */

#ifndef OPERATION_H
#define	OPERATION_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum { ADD=0, EMPTY, HELP, LIST, RUN, VERSION } operation;
void test_operation( int *passed, int *failed );
operation operation_value( const char *value );


#ifdef	__cplusplus
}
#endif

#endif	/* OPERATION_H */

