#ifndef _aatree_H
#define _aatree_H

typedef struct aatree_struct aatree;
typedef int (*compare_func)( void *, void* );
typedef void (*print_node_func)( void *, void * );
typedef void (*aatree_dispose_func)(void*);
aatree *aatree_create( compare_func cf, int limit, aatree_dispose_func disp );
void aatree_dispose( aatree *t );
void *aatree_find( aatree *t, void *x );
void *aatree_max( aatree *t );
void *aatree_min( aatree *t );
void *aatree_add( aatree *t, void *item );
int aatree_delete( aatree *t, void *item );
int aatree_empty( aatree*t );
int aatree_verify_tree( aatree *t );
void aatree_print_tree( aatree *t, print_node_func pn, void *data );
#ifdef MVD_TEST
void aatree_test( int *passed, int *failed );
#endif
#endif 

