#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "plugin_log.h"
#include "unicode/uchar.h"
#include "node.h"
#include "hashtable.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif
struct bucket
{
    UChar c;
    node *v;
    struct bucket *next;
};
struct hashtable_struct
{
    struct bucket **items;
    int nbuckets;
    int nitems;
};
/**
 * Create a hashtable by conversion from a list of child-nodes
 * @param children add these nodes to the hashtable for starters
 * @param str the text we represent
 * @param log the log to report errors to
 * @return an initialised hashtable
 */
hashtable *hashtable_create( node *parent, UChar *str, plugin_log *log )
{
    hashtable *ht = calloc( 1, sizeof(hashtable) );
    if ( ht != NULL )
    {
        int nnodes = node_num_children( parent );
        ht->nbuckets = nnodes*2;
        ht->items = calloc( ht->nbuckets, sizeof(struct bucket*) );
        if ( ht->items != NULL )
        {
            int res = 1;
            node_iterator *iter = node_children( parent, log );
            if ( iter != NULL )
            {
                while ( res && node_iterator_has_next(iter) )
                {
                    node *temp = node_iterator_next( iter );
                    node_clear_next( temp );
                    res = hashtable_add( ht, temp, str );
                }
                node_iterator_dispose( iter );
            }
            else
                res = 0;
            if ( !res )
            {
                hashtable_dispose( ht );
                ht = NULL;
            }
        }
        else
            plugin_log_add(log,"failed to allocate %d buckets\n",ht->nbuckets);
    }
    else
        plugin_log_add(log,"hashtable: failed to allocate\n");
    return ht;
}
/**
 * Dispose of the hashtable and its children
 * @param htthe hashtable to dispose of
 */
void hashtable_dispose( hashtable *ht )
{
    int i = 0;
    for ( i=0;i<ht->nbuckets;i++ )
    {
        struct bucket *b = ht->items[i];
        while ( b != NULL )
        {
            struct bucket *next = b->next;
            node_dispose( b->v );
            free( b );
            b = next;
        }
    }
    if ( ht->items != NULL )
        free( ht->items );
    free( ht );
}
static int hash( UChar key, int nbuckets )
{
    return key % nbuckets;
}
/**
 * Debug routine only
 * @param ht the hashtable to print
 * @param str the text we represent
 */
static void hashtable_print( hashtable *ht, UChar *str )
{
    node **nodes = calloc(ht->nitems,sizeof(node*));
    hashtable_to_array( ht, nodes );
    int i;
    for ( i=0;i<ht->nitems;i++ )
        printf("%x ",node_first_char(nodes[i],str));
    printf("\n");
}
/**
 * Expand this hashtable
 * @param ht the hashtable to expand
 * @return 1 if it worked
 */
static int hashtable_expand( hashtable *ht )
{
    int newnbuckets = ht->nbuckets *3/2;
    struct bucket **newitems = calloc( newnbuckets, sizeof(struct bucket*) );
    if ( newitems != NULL )
    {
        int i;
        for ( i=0;i<ht->nbuckets;i++ )
        {
            struct bucket *b = ht->items[i];
            while ( b != NULL )
            {
                int index = hash(b->c, newnbuckets);
                struct bucket *next = b->next;
                b->next = NULL;
                if ( newitems[index] == NULL )
                    newitems[index] = b;
                else    // append
                {
                    struct bucket *b2 = newitems[index];
                    while ( b2->next != NULL )
                        b2 = b2->next;
                    b2->next = b;
                }
                b = next;
            }
        }
        free( ht->items );
        ht->items = newitems;
        ht->nbuckets = newnbuckets;
        return 1;
    }
    return 0;
}
/**
 * Add an item to the hashtable
 * @param ht the table
 * @param child the node
 * @return 1 if it didn't need expansion or the expansion succeeded
 */
int hashtable_add( hashtable *ht, node *child, UChar *str )
{
    int res = 1;
    struct bucket *b = calloc( 1, sizeof(struct bucket) );
    if ( b != NULL )
    {
        if ( ht->nitems+1 > (ht->nbuckets*8+10)/10 )
            res = hashtable_expand(ht);
        if ( res )
        {
            b->v = child;
            b->c = node_first_char( child, str );
            int index = hash( b->c, ht->nbuckets);
            if ( ht->items[index] == NULL )
                ht->items[index] = b;
            else
            {
                struct bucket *b2 = ht->items[index];
                while ( b2->next != NULL )
                    b2 = b2->next;
                b2->next = b;
            }
            ht->nitems++;
        }
    }
    return res;
}
/**
 * Remove a node form the hashtable
 * @param ht the table to remove it from
 * @param first_char the first char of the entry to remove
 * @return 1 if it was removed
 */
int hashtable_remove( hashtable *ht, node *v, UChar first_char )
{
    int index = hash( first_char, ht->nbuckets);
    struct bucket *b = ht->items[index];
    if ( b != NULL )
    {
        struct bucket *last = b;
        while ( b != NULL && b->c != first_char )
        {
            last = b;
            b = b->next;
        }
        if ( b != NULL ) // found it
        {
            if ( last != b )
                last->next = b->next;
            else
                ht->items[index] = b->next;
            free( b );
            ht->nitems--;
            return 1;
        }
    }
    return 0;
}
/**
 * Replace one node with another
 * @param ht the hashtable to do it in
 * @param str the text we represent
 * @param u the node to replace it with
 * @return 1 if it worked
 */
int hashtable_replace( hashtable *ht, UChar *str, node *v, node *u )
{
    if ( hashtable_remove(ht,v,node_first_char(u,str)) )
        return hashtable_add(ht,u,str);
    else
        return 0;
}
/**
 * Get a node from the table. Has to be fast.
 * @param ht the hashtable in question
 * @param c the first char 
 * @return the node or NULL if not found
 */
node *hashtable_get( hashtable *ht, UChar c )
{
    int index = hash( c, ht->nbuckets );
    struct bucket *b = ht->items[index];
    while ( b != NULL && b->c != c )
        b = b->next;
    if ( b != NULL )
        return b->v;
    else
        return NULL;
}
/**
 * Convert a hashtable's values to an array
 * @param ht the hashtable to convert
 * @param nodes an array of just the right size
 */
void hashtable_to_array( hashtable *ht, node **nodes )
{
    int i,j;
    for ( i=0,j=0;i<ht->nbuckets;i++ )
    {
        struct bucket *b = ht->items[i];
        while ( b != NULL )
        {
            nodes[j++] = b->v;
            b = b->next;
        }
    }
}
/**
 * How many nodes are in the table?
 * @param ht the hashtable object
 * @return the number of current nodes stored
 */
int hashtable_size( hashtable *ht )
{
    return ht->nitems;
}
#ifdef HASHTABLE_TEST
extern UChar *str;
int main( int argc, char **argv )
{
    node *root = node_create(0,0);
    int i,slen = strlen(str)-1;
    for ( i=0;i<slen;i++ )
    {
        node *v = node_create(i,1);
        if ( find_child(root,str[i])==NULL )
            node_add_child( root, v );
        else
            node_dispose( v );
    }
    for ( i=0;i<slen;i++ )
    {
        node *v = find_child(root,str[i]);
        if ( v == NULL )
            printf("couldn't find %c\n",str[i]);
    }
    node_dispose( root );
}
#endif