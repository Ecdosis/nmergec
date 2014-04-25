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
    // version to follow to this location
    int version;
    // global position within that version
    int vindex;
};
typedef struct location_struct location;
int location_set( location *l, int version, card *start );
int location_update( location *l, int version, card *start );
#ifdef	__cplusplus
}
#endif

#endif	/* LOCATION_H */

