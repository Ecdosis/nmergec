/* 
 * File:   verify.h
 * Author: desmond
 *
 * Created on June 12, 2013, 10:14 AM
 */

#ifndef VERIFY_H
#define	VERIFY_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct verify_struct verify;
verify *verify_create( dyn_array *pairs, int nversions );
void verify_dispose( verify *v );
int verify_check( verify *v, int node_stats );
#ifdef MVD_TEST
void test_verify( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* VERIFY_H */

