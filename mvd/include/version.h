/* 
 * File:   version.h
 * Author: desmond
 *
 * Created on January 25, 2013, 10:00 AM
 */

#ifndef VERSION_H
#define	VERSION_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct version_struct version;
version *version_create( UChar *id, UChar *description );
void version_dispose( version *v );
UChar *version_description( version *v );
UChar *version_id( version *v );
int version_datasize( version *v, int old, char *encoding );
#ifdef MVD_TEST
void test_version( int *passed, int *failed );
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* VERSION_H */

