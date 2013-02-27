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

MVD *mvdfile_internalise( char *data, int len );
char *mvdfile_externalise( MVD *mvd, int *len, int old );
MVD *mvdfile_load( char *file );
int mvdfile_save( MVD *mvd, char *file, int old );
#ifdef MVD_TEST
void test_mvdfile( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* MVDFILE_H */

