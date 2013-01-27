/* 
 * File:   pair.h
 * Author: desmond
 *
 * Created on January 16, 2013, 4:40 PM
 */

#ifndef PAIR_H
#define	PAIR_H

#ifdef	__cplusplus
extern "C" {
#endif
#define TRANSPOSE_MASK 0xC0000000
#define INVERSE_MASK 0x0FFFFFFF
#define PARENT_FLAG 0x80000000
#define CHILD_FLAG 0x40000000
#define DUFF_PID -1
#define NULL_PID 0

typedef struct pair_struct pair;
pair *pair_create_basic( bitset *versions, unsigned char *data, int len );
pair *pair_create_parent( bitset *versions, unsigned char *data, int len );
pair *pair_create_child( bitset *versions );
void pair_dispose( pair *p );
pair *pair_set_parent( pair *p, pair *parent );
pair *pair_add_child( pair *p, pair *child );
int pair_is_child( pair *p );
int pair_is_parent( pair *p );
pair *pair_parent( pair *p );
int pair_set_id( pair *p, int id );
int pair_id( pair *p );
link_node *pair_first_child( pair *p );
int pair_size( pair *p, int versionSetSize );
int pair_datasize( pair *p );
int pair_serialise( pair *p, unsigned char *data, int len, int offset, 
    int setSize, int dataOffset, int dataTableOffset, int parentId );
int pair_is_hint( pair *p );
bitset *pair_versions( pair *p );

#ifdef	__cplusplus
}
#endif

#endif	/* PAIR_H */

