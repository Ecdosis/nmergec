/* 
 * File:   mvdfile.h
 * Author: desmond
 *
 * Created on January 16, 2013, 8:33 AM
 */

#ifndef MVDFILE_H
#define	MVDFILE_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MVD_MAGIC_OLD 0xDEADC0DEU
#define MVD_MAGIC_NEW 0x600DC0DEU
#define MVD_MAGIC_OLD_STR "\336\255\300\336"
#define MVD_MAGIC_NEW_STR "\140\015\300\336"
#define MVD_MAGIC_LEN 4

MVD *mvdfile_internalise( char *data, int len );
char *mvdfile_externalise( MVD *mvd, int *len, int old );
MVD *mvdfile_load( char *file );
int mvdfile_save( MVD *mvd, char *file, int old );
#ifdef MVD_TEST
int test_mvdfile( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* MVDFILE_H */

