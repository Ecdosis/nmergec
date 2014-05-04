/* 
 * File:   location.h
 * Author: desmond
 *
 * Created on 23 April 2014, 1:28 PM
 */

#ifndef LOCATION_H
#define	LOCATION_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Keep track of a location within the variant graph
 */
struct location_struct
{
    // cached card location
    card *current;
    // position within current card
    int pos;
};
typedef struct location_struct location;

#ifdef	__cplusplus
}
#endif

#endif	/* LOCATION_H */

