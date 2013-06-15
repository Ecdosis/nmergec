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
verify *verify_create( MVD *m );
void verify_dispose( verify *v );
int verify_check( verify *v );


#ifdef	__cplusplus
}
#endif

#endif	/* VERIFY_H */

