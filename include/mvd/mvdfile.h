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

MVD *mvdfile_internalise( char *data, int len );
MVD *mvd_load( char *file );

#ifdef	__cplusplus
}
#endif

#endif	/* MVDFILE_H */
