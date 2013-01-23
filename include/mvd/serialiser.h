/* 
 * File:   serialiser.h
 * Author: desmond
 *
 * Created on January 22, 2013, 11:32 AM
 */

#ifndef SERIALISER_H
#define	SERIALISER_H

#ifdef	__cplusplus
extern "C" {
#endif
void write_int( unsigned char *data, int p, int value );
void write_short( unsigned char *data, int p, short value );
int write_string( unsigned char *data, int p, unsigned char *value ); 


#ifdef	__cplusplus
}
#endif

#endif	/* SERIALISER_H */

