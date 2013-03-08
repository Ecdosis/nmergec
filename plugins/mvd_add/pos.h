/* 
 * File:   pos.h
 * Author: desmond
 *
 * Created on March 4, 2013, 9:53 AM
 */

#ifndef POS_H
#define	POS_H

#ifdef	__cplusplus
extern "C" {
#endif

// describes a character-position in the tree
typedef struct pos_struct pos;
struct pos_struct
{
    node *v;
    int loc;
};


#ifdef	__cplusplus
}
#endif

#endif	/* POS_H */

