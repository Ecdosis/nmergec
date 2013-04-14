#ifndef _aatree_H
#define _aatree_H

typedef struct aatree_struct aatree;
typedef int (*compare_func)( void *, void* );
typedef void (*aatree_dispose_func)(void*);
aatree *aatree_create( compare_func cf, int limit );
void aatree_dispose( aatree *t, aatree_dispose_func disp );
void *aatree_find( aatree *t, void *x );
void *aatree_max( aatree *t );
void *aatree_min( aatree *t );
void *aatree_add( aatree *t, void *item );
int aatree_delete( aatree *t, void *item );
int aatree_empty( aatree*t );
void aatree_test( int *passed, int *failed );
#endif 

