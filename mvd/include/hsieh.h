/* 
 * File:   hsieh.h
 * Author: desmond
 *
 * Created on January 19, 2013, 4:33 AM
 */

#ifndef HSIEH_H
#define	HSIEH_H

#ifdef	__cplusplus
extern "C" {
#endif

uint32_t hsieh_hash (const char * data, int len);
#ifdef MVD_TEST
void test_hsieh( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* HSIEH_H */

