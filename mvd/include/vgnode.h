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
vgnode *vgnode_create();
void vgnode_dispose( vgnode *n );
int vgnode_add_incoming( vgnode *n, pair *p );
int vgnode_add_outgoing( vgnode *n, pair *p );
int vgnode_balanced( vgnode *n );
int vgnode_wants(vgnode *n, pair *p );
char *vgnode_tostring( vgnode *n );
bitset *vgnode_versions( vgnode *vg );
int vgnode_outdegree( vgnode *vg );
int vgnode_indegree( vgnode *vg );
bitset *vgnode_outgoing( vgnode *vg );
bitset *vgnode_incoming( vgnode *vg );
void vgnode_clear_version( vgnode *vg, int version );
#ifdef	__cplusplus
}
#endif

#endif	/* VGNODE_H */

