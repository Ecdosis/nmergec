/* 
 * File:   group.h
 * Author: desmond
 *
 * Created on January 22, 2013, 3:20 PM
 */

#ifndef GROUP_H
#define	GROUP_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct group_struct group;
group *group_create( int id, int parent );
void group_dispose( group *g );
void group_set_parent( group *g, int parent );
int group_id( group *g );
int group_parent( group *g );

#ifdef	__cplusplus
}
#endif

#endif	/* GROUP_H */

