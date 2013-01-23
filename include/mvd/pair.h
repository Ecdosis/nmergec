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

typedef struct pair_struct pair;
pair *pair_create_basic( bitset *versions, unsigned char *data, int len );
pair *pair_create_parent( bitset *versions, unsigned char *data, int len );
pair *pair_create_child( bitset *versions );
void pair_dispose( pair *p );
pair *pair_set_parent( pair *p, pair *parent );
pair *pair_add_child( pair *p, pair *child );
int pair_is_child( pair *p );

#ifdef	__cplusplus
}
#endif

#endif	/* PAIR_H */

