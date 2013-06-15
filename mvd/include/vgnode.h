/* 
 * File:   vgnode.h
 * Author: desmond
 *
 * Created on June 12, 2013, 11:23 AM
 */

#ifndef VGNODE_H
#define	VGNODE_H

#ifdef	__cplusplus
extern "C" {
#endif
#define VGNODE_START 1
#define VGNODE_BODY 2
#define VGNODE_END 3

typedef struct vgnode_struct vgnode;
vgnode *vgnode_create( int kind );
void vgnode_dispose( vgnode *n );
void vgnode_add_incoming( vgnode *n, pair *p );
void vgnode_add_outgoing( vgnode *n, pair *p );
int vgnode_verify( vgnode *n, bitset *all );
void vgnode_outversions( vgnode *n, char *dst, int len );
void vgnode_inversions( vgnode *n, char *dst, int len );
bitset *vgnode_overhang( vgnode *n );
int vgnode_check_incoming( vgnode *n, bitset *pv );

#ifdef	__cplusplus
}
#endif

#endif	/* VGNODE_H */

