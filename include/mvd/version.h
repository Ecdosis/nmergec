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
version *version_create( char *id, char *description );
void version_dispose( version *v );
char *version_description( version *v );
char *version_id( version *v );
int version_datasize( version *v, int old );

#ifdef	__cplusplus
}
#endif

#endif	/* VERSION_H */

