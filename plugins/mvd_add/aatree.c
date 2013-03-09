#include <stdlib.h>
#include <stdio.h>
#include "aatree.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

/**
 * Arne Andersson style balanced binary tree with a size limitation
 */
int node_frees = 0;
int num_inserts = 0;
// tree-node
typedef struct aanode_struct aanode;
struct aanode_struct
{
    void *element;
    aanode *left;
    aanode *right;
    int level;
};
// aatree class
struct aatree_struct
{
	aanode *root;
	compare_func cf;
    aanode *null_node;
	aanode *delete_ptr;
    aanode *last_ptr;
    // shortcut to finding minimum
    aanode *min;
    // maximum size of tree
    int limit;
    // number of nodes in tree
    int size;
};
/**
 * Create an aatree instance
 * @param cf the comparison function for elements in the tree
 * @param limit maximum size of the tree
 * @return an aatree object
 */
aatree *aatree_create( compare_func cf, int limit )
{
    aatree *t = calloc( 1, sizeof(aatree) );
	if ( t != NULL )
	{
		t->null_node = calloc( 1, sizeof(aanode) );
        if ( t->null_node != NULL )
        {
            t->null_node->left = t->null_node->right = t->null_node;
            t->null_node->level = 0;
            t->root = t->null_node;
            t->limit = limit;
            t->cf = cf;
        }
        else
            fprintf(stderr,"aatree: failed to allocate object\n");
	}
	else
		fprintf(stderr,"failed to inialise aatree\n");
    return t;
}
/**
 * Dispose of a node and all its children
 * @param n the node to dispose
 * @param null_node the null_node: don't destroy!
 */
void aanode_dispose( aanode *n, aanode *null_node )
{
    if ( n->left != null_node )
        aanode_dispose( n->left, null_node );
    if ( n->right != null_node )
        aanode_dispose( n->right, null_node );
    node_frees++;
    free( n );
}
/**
 * Dispose of the entire tree
 * @param t the tree in question
 */
void aatree_dispose( aatree *t )
{
    aanode_dispose( t->root, t->null_node );
    free( t->null_node );
    free( t );
}
/**
 * Find an element in the tree
 * @param t the tree in question
 * @param n the node to start from (for recursion)
 * @param x the value to search for
 * @return the node it was found in or NULL if not found 
 */
static void *aatree_search( aatree *t, void *x )
{
    aanode *current = t->root;
    t->null_node->element = x;

    for( ; ; )
    {
        if ( t->cf(x,current->element) < 0 )
            current = current->left;
        else if( t->cf(x,current->element) > 0 ) 
            current = current->right;
        else if( current != t->null_node )
            return current->element;
        else
            break;
    }
    return NULL;
}
/**
 * Get the element associated with this node
 * @param n the node in question
 * @return the element
 */
static void *aanode_get_element( aanode *n )
{
    return n->element;
}
/**
 * Find the minimum in O(log N)
 * @param t the tree to search
 * @param n the node to search from (for recursion)
 * @return the node it is found in or the null node if the tree was empty
 */
static aanode *aatree_find_min( aatree *t, aanode *n )
{
    if ( n == t->null_node )
        return t->null_node;
    else if ( n->left == t->null_node )
        return n;
    else
        return aatree_find_min( t, n->left );
}
/**
 * Find the maximum in O(log N)
 * @param t the tree to search
 * @param n the node to search from (for recursion)
 * @return the node it is found in or the null node if the tree was empty
 */
static aanode *aatree_find_max( aatree *t, aanode *n )
{
    if ( n != t->null_node )
        while ( n->right != t->null_node )
            n = n->right;
    return n;
}
/**
 * Perform a rotate between a node (K2) and its left child 
 * @param K2 the current root of the subtree
 * @return the new root of the subtree
 */
static aanode *rotate_left( aanode *K2 )
{
    aanode *K1;
    K1 = K2->left;
    K2->left = K1->right;
    K1->right = K2;
    return K1;  
}
/** 
 * Perform a rotate between a node (K1) and its right child
 * @param K1 the current root of the subtree
 * @return the new subtree root
 */
static aanode *rotate_right( aanode *K1 )
{
    aanode *K2;
    K2 = K1->right;
    K1->right = K2->left;
    K2->left = K1;
    return K2;
}
/** 
 * If n's left child is on the same level as n, perform a rotation
 * @param n the current root of the subtree
 * @return the new root
 */
static aanode *skew( aanode *n )
{
    if ( n->left->level == n->level )
        n = rotate_left( n );
    return n;
}
/** 
 * If n's rightmost grandchild is on the same level, rotate right child up 
 * @param n the node in question
 */
static aanode *split( aanode *n )
{
    if ( n->right->right->level == n->level )
    {
        n = rotate_right( n );
        n->level++;
    }
    return n;
}
/**
 * Insert an item in the tree
 * @param t the tree object
 * @param n the node to insert starting from
 * @param item the item to insert
 * @return the new subtree root
 */
static aanode *aatree_insert( aatree *t, aanode *n, void *item, int *exists )
{
    *exists = 0;
    if ( n == t->null_node )
    {
        n = calloc( 1, sizeof(aanode) );
        if ( n != NULL )
        {
            n->element = item;
            n->left = t->null_node;
            n->right = t->null_node;
            n->level = 1;
            if ( t->root == t->null_node )
                t->root = n;
            num_inserts++;
        }
        else
            fprintf(stderr,"aatree: failed to allocate new node\n");
    }
    else if ( t->cf(item,n->element)<0 )
        n->left = aatree_insert( t, n->left, item, exists );
    else if ( t->cf(item,n->element) > 0 )
        n->right = aatree_insert( t, n->right, item, exists );
    else
        *exists = 1;
    n = skew( n );
    n = split( n );
    return n;
}
/**
 * Remove an item from the tree
 * @param t the tree in question
 * @param n the node to start deleting from
 * @param item the item to remove
 * @return the new root node
 */
static aanode *aatree_remove( aatree *t, aanode *n, void *item )
{
    if ( n != t->null_node )
    {
        // Step 1: Search down tree
        // set last_ptr and delete_ptr
        t->last_ptr = n;
        if ( t->cf(item,n->element)<0 )
            n->left = aatree_remove( t, n->left, item );
        else
        {
            t->delete_ptr = n;
            n->right = aatree_remove( t, n->right, item );
        }
        // Step 2: If at the bottom of the tree and
        // item is present, we remove it
        if ( n == t->last_ptr )
        {
            if ( t->delete_ptr != t->null_node &&
                t->cf(item,t->delete_ptr->element)==0 )
            {
                t->delete_ptr->element = n->element;
                //t->delete_ptr = t->null_node;
                n = n->right;
                free( t->last_ptr );
            }
            else
                printf("error!\n");
        }
        // Step 3: Otherwise, we are not at the bottom; rebalance
        else if ( n->left->level < n->level - 1 ||
                n->right->level < n->level - 1 )
        {
            if ( n->right->level > --n->level )
                n->right->level = n->level;
            n = skew( n );
            n->right = skew( n->right );
            n->right->right = skew( n->right->right );
            n = split( n );
            n->right = split( n->right );
        }
    }
    return n;
}
/**
 * Add an item to the aatree
 * @param t the tree object
 * @param item the item to add
 * @return 1 if it was added else 0
 */
int aatree_add( aatree *t, void *item )
{
    int res = 1;
    int exists = 0;
    if ( t->size == t->limit )
    {
        void *elem = aanode_get_element( t->min );
        if ( t->cf(item,elem)>0 )
        {
            t->root = aatree_remove( t, t->root, t->min->element );
            t->root = aatree_insert(t,t->root,item,&exists);
            t->min = aatree_find_min( t, t->root );
            if ( exists )
            {
                t->size--;
                res = 0;
            }
        }
        else
            res = 0;
    }
    else 
    {
        t->root = aatree_insert(t,t->root,item,&exists);
        if ( !exists )
        {
            t->min = aatree_find_min( t, t->root );
            t->size++;
        }
        else
            res = 0;
    }
    return res;
}
/**
 * Get the current maximum
 * @param t the tree in question
 * @return the maximum element
 */
void *aatree_max( aatree *t )
{
    return aanode_get_element(aatree_find_max(t,t->root));
}
/**
 * Get the current minimum
 * @param t the tree in question
 * @return the minimum element
 */
void *aatree_min( aatree *t )
{
    return (t->min!=NULL)?aanode_get_element(t->min):NULL;
}
/**
 * Simply delete an item from the tree
 * @param t the tree in question
 * @param item the item to find and remove
 * @return 1 if it was removed else 0
 */
int aatree_delete( aatree *t, void *item )
{
    t->root = aatree_remove( t, t->root, item );
    if ( t->root != t->null_node )
    {
        t->size--;
        return 1;
    }
    else
        return 0;
}
/**
 * Check that the tree's nodes are correctly ordered
 * @param t the tree to verify
 * @param n the node to start from (recursive)
 */
static int aatree_verify( aatree *t, aanode *n )
{
    int res = 1;
    if ( n->left != t->null_node 
        && t->cf(n->left->element,n->element)>0 )
        res = 0;
    else if ( n->right != t->null_node 
        && t->cf(n->right->element,n->element)<0 )
        res = 0;
    if ( res && n->left != t->null_node )
        res = aatree_verify( t, n->left );
    if ( res && n->right != t->null_node )
        res = aatree_verify( t, n->right );
    return res;
}
/**
 * Compare function for testing
 * @param a pointer to an int
 * @param b another int pointer
 * @return 0,1,-1 as per strcmp
 */
static int compare( int *a, int *b )
{
	if ( *a > *b )
		return 1;
	else if ( *a < *b )
		return -1;
	else
		return 0;
}
/**
 * Test an aatree
 * @param passed VAR param number of passed tests
 * @param failed number of failed tests
 */
void aatree_test( int *passed, int *failed )
{
    srand( (long)time(NULL) );
	int i;
    int values[100];
    aatree *t = aatree_create( (compare_func) compare, 50 );
	i=0;
    int N = sizeof(values)/sizeof(int);
    for ( i=0;i<N;i++ )
	{
        values[i] = rand()%100;
		aatree_add( t, &values[i] );
	}
    if ( aatree_verify(t,t->root) )
    {
        *passed += 1;
        printf("tree was OK. size=%d\n",t->size);
    }
    else
    {
        *failed += 1;
        printf("tree was incorrect. size=%d\n",t->size);
    }
    int saved_size = t->size;
    aatree_dispose( t );
    if ( node_frees != saved_size )
    {
        printf("aatree: failed to free %d nodes\n",(saved_size-node_frees));
        *failed += 1;
    }
    else
        *passed += 1;
}