/* 
 * File:   bitset.h
 * Author: desmond
 *
 * Created on January 16, 2013, 2:05 PM
 */

#ifndef BITSET_H
#define	BITSET_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct bitset_struct bitset;
bitset *bitset_create();
bitset *bitset_create_exact( int bits );
bitset *bitset_clone( bitset *bs );
void *bitset_dispose( bitset *bs );
bitset *bitset_set( bitset *bs, int i );
int bitset_get( bitset *bs, int i );
bitset *bitset_or( bitset *bs, bitset *other );
void bitset_and( bitset *bs, bitset *other );
int bitset_cardinality( bitset *bs );

#ifdef	__cplusplus
}
#endif

#endif	/* BITSET_H */

