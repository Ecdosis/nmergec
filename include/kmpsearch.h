/* 
 * File:   kmpsearch.h
 * Author: desmond
 *
 * Created on January 11, 2013, 3:58 PM
 */

#ifndef KMPSEARCH_H
#define	KMPSEARCH_H

#ifdef	__cplusplus
extern "C" {
#endif

int search( char *text, int offset, char *pattern );
#ifdef MVD_DEBUG
int test_kmpsearch( int *passed, int *failed );
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* KMPSEARCH_H */

