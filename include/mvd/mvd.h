/* 
 * File:   mvd.h
 * Author: desmond
 *
 * Created on January 14, 2013, 7:28 AM
 */

#ifndef MVD_H
#define	MVD_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct MVD_struct MVD;
int mvd_datasize( MVD *mvd );
int mvd_serialise( MVD *mvd, char *data );
   



#ifdef	__cplusplus
}
#endif

#endif	/* MVD_H */

