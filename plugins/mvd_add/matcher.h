/* 
 * File:   matcher.h
 * Author: desmond
 *
 * Created on March 2, 2013, 7:52 AM
 */

#ifndef MATCHER_H
#define	MATCHER_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct matcher_struct matcher;
matcher *matcher_create( alignment *a, aatree *pq, linkpair *pairs );
void matcher_dispose( matcher *m );
int matcher_align( matcher *m );
match *matcher_get_mum( matcher *m );


#ifdef	__cplusplus
}
#endif

#endif	/* MATCHER_H */

