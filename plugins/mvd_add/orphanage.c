#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "card.h"
#include "orphanage.h"
#include "hashmap.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define KEYLEN 32
/**
 * An orphanage is a collection of parents and their (initially) 
 * lost children. As we read through a list of cards we store 
 * the parents and children here and it is the job of this class 
 * to match children to their parents by ID (stored in the pair).
 */
struct orphanage_struct
{
    // children withOUT parents
    hashmap *orphans;
    // parents WITH or WITHOUT children
    hashmap *parents;
    // children WITH parents
    hashmap *children;
    // current ID to be incremented on request
    int currentID;
};
/**
 * Create an orphanage object
 * @return an initialised orphanage
 */
orphanage *orphanage_create()
{
    orphanage *o = calloc( 1, sizeof(orphanage) );
    if ( o != NULL )
    {
        o->orphans = hashmap_create( 20, 1 );
        o->children = hashmap_create( 20, 1 );
        o->parents = hashmap_create( 20, 1 );
        o->currentID = 0;
        if ( o->orphans == NULL 
            || o->parents == NULL || o->children == NULL )
        {
            orphanage_dispose( o );
            o = NULL;
        }
    }
    else
        fprintf( stderr, "orphanage: failed to allocate object\n" );
    return o;
}
/**
 * Free up the memory taken by an orphanage object
 * @param o the orphanage in question
 */
void orphanage_dispose( orphanage *o )
{
    if ( o->orphans != NULL )
        hashmap_dispose( o->orphans, NULL );
    if ( o->parents != NULL )
        hashmap_dispose( o->parents, NULL );
    if ( o->children != NULL )
        hashmap_dispose( o->children, NULL );
    free( o );
}
/**
 * Get the next ID
 * @param o the orphanage instance
 * @return the new ID, never before issued
 */
int orphanage_next_id( orphanage *o )
{
    return ++o->currentID;
}
/**
 * Add a parent to the register of parents
 * @param o the orphanage instance
 * @param parent the parent desiring orphans to adopt
 * @return 1 if successful
 */
int orphanage_add_parent( orphanage *o, card *parent )
{
    int res = 1;
    // look in orphans for children with this parent
    int i,n_orphans = hashmap_size( o->orphans );
    UChar **keys = calloc( n_orphans, sizeof(UChar*) );
    if ( keys != NULL )
    {
        hashmap_to_array( o->orphans, keys );
        pair *pp = card_pair(parent);
        int pid = pair_id( pp );
        dyn_array *da = dyn_array_create( 20 );
        if ( da != NULL )
        {
            // look for orphans
            for ( i=0;i<n_orphans;i++ )
            {
                card *child = hashmap_get( o->orphans, keys[i] );
                pair *cp = card_pair( child );
                pair *cp_p = pair_parent( cp );
                int pp_id = pair_id( cp_p );
                if ( pp_id == pid )
                    dyn_array_add( da, child );
            }
            card **children = calloc( dyn_array_size(da)+1, 
                sizeof(card*) );
            if ( children != NULL )
            {
                UChar u_key[KEYLEN];
                for ( i=0;i<dyn_array_size(da);i++ )
                {
                    children[i] = dyn_array_get(da,i);
                    int cid = pair_id(card_pair(children[i]));
                    calc_ukey( u_key, cid, KEYLEN );
                    hashmap_remove( o->orphans, u_key, NULL );
                    res = hashmap_put( o->children, u_key, parent );
                    if ( !res )
                        break;
                }
                // even if parent has no children we put this here
                if ( res )
                {
                    calc_ukey( u_key, pid, KEYLEN );
                    res = hashmap_put( o->parents, u_key, children );
                }
                else
                    free( children );
            }
            dyn_array_dispose( da );
        }
        free( keys );
    }
    return res;
}
/**
 * Add a child to the register of orphans
 * @param o the orphanage instance
 * @param child the child looking for a parent
 * @return 1 if successful else 0 if it was already there or an error
 */
int orphanage_add_child( orphanage *o, card *child )
{
    int res = 0;
    // first look in parents for this child's parent ID
    pair *p = card_pair(child);
    int cid = pair_id(p);
    if ( cid > o->currentID )
        o->currentID = cid;
    if ( pair_is_child(p) )
    {
        pair *pp = pair_parent(p);
        if ( pp != NULL )
        {
            int pid = pair_id(pp);
            UChar u_key[KEYLEN];
            calc_ukey( u_key, pid, KEYLEN );
            if ( pid > o->currentID )
                o->currentID = pid;
            if ( hashmap_contains(o->parents,u_key) )
            {
                card **kids = hashmap_get(o->parents,u_key);
                if ( kids != NULL )
                {
                    int i = 0;
                    while ( kids[i] != NULL )
                    {
                        if ( kids[i++] == child )
                            break;
                    }
                    // did we NOT find that child?
                    if ( kids[i] == NULL )
                    {
                        // one for new kid, one for terminating NULL
                        card **new_kids = calloc( i+2, sizeof(card*));
                        if ( new_kids != NULL )
                        {
                            i = 0;
                            // copy old ones over
                            while ( kids[i] != NULL )
                                new_kids[i++] == kids[i];
                            new_kids[i] = child;
                            res = hashmap_remove( o->parents, u_key, NULL/*free*/ );
                            if ( res )
                                res = hashmap_put( o->parents, u_key, new_kids );
                            if ( res )
                            {
                                // update children hash
                                UChar c_key[KEYLEN];
                                calc_ukey( c_key, cid, KEYLEN );
                                res = hashmap_put( o->children, c_key, child );
                            }
                        }
                        else
                        {
                            fprintf(stderr,
                                "orphanage: failed to reallocate children\n");
                        }
                    }
                    // else nothing to do: already present
                }
                // else shouldn't happen: res will be 0
            }
            else    // parent not present: add to orphans
            {
                calc_ukey( u_key, cid, KEYLEN );
                res = hashmap_put( o->orphans, u_key, child );
            }
        }
        // else shouldn't happen: res will be 0
    }
    // else shouldn't happen: res will be 0
    return res;
}
/**
 * Does every little boy or girl have a parent?
 * @param o the orphanage in question
 * @return 1 if there are no orphans
 */
int orphanage_is_empty( orphanage *o )
{
    return hashmap_size(o->orphans)==0;
}
/**
 * Find a child's parent
 * @param o the orphanage instance
 * @param child the child looking for a Mummy or a Daddy
 * @return the adoptive parent or NULL if not found
 */
card *orphanage_get_parent( orphanage *o, card *child )
{
    pair *p = card_pair(child);
    if ( pair_is_child(p) )
    {
        UChar u_key[KEYLEN];
        calc_ukey( u_key, pair_id(p), KEYLEN );
        if ( hashmap_contains( o->children, u_key) )
            return hashmap_get( o->children, u_key );
    }
    return NULL;
}
/**
 * Get a complete set of a parent's children
 * @param o the orphanage instance
 * @param parent the parent whose children are desired
 * @param children an array of cards
 * @param size the number of slots in children-1 (last one is NULL)
 */
void orphanage_get_children( orphanage *o, card *parent, 
    card **children, int size )
{
    pair *p = card_pair(parent);
    UChar u_key[KEYLEN];
    calc_ukey( u_key, pair_id(p), KEYLEN );
    card **kids = hashmap_get( o->parents, u_key );
    int i = 0;
    if ( children != NULL )
    {
        while ( kids[i] != NULL && i < size )
            children[i] = kids[i++];
    }
}
/**
 * Count the children for a given parent
 * @param o the orphanage instance
 * @param parent the parent whose children are sought
 * @return the number of children he/she has
 */
int orphanage_count_children( orphanage *o, card *parent )
{
    pair *p = card_pair(parent);
    UChar u_key[KEYLEN];
    calc_ukey( u_key, pair_id(p), KEYLEN );
    card **children = hashmap_get( o->parents, u_key );
    int num_children = 0;
    if ( children != NULL )
    {
        while ( children[num_children] != NULL )
            num_children++;
    }
    return num_children;
}
/**
  * Remove a parent and all its children. 
  * @param o the orphanage in question
  * @param parent the parent and children to remove
  * @return 1 on success
  */
int orphanage_remove_parent( orphanage *o, card *parent )
{
    int res = 0;
    pair *p = card_pair(parent);
    UChar u_key[KEYLEN];
    calc_ukey( u_key, pair_id(p), KEYLEN );
    card **children = hashmap_get( o->parents, u_key );
    if ( children != NULL )
    {
        int i = 0;
        while ( children[i] != NULL )
        {
            UChar c_key[KEYLEN];
            pair *c = card_pair(children[i]);
            calc_ukey( c_key, pair_id(c), KEYLEN );
            res = hashmap_remove( o->children, c_key, NULL );
            if ( !res )
                break;
            i++;
        }
    }
    if ( res )
        res = hashmap_remove( o->parents, u_key, NULL );
    return res;
}
/**
 * Get a simple array of all the children in the orphanage
 * @param o the orphanage to collect them from
 * @param num VAR param update with number of children found (may be 0)
 * @param success VAR param set to 1 if it worked els set to 0
 * @return all the children as an allocated array of cards or NULL if none
 */
card **orphanage_all_children( orphanage *o, int *num, int *success )
{
    card **children = NULL;
    *success = 0;
    *num = hashmap_size( o->children );
    if ( *num > 0 )
    {
        UChar **keys = calloc( *num, sizeof(UChar*) );
        if ( keys != NULL )
        {
            int i;
            hashmap_to_array( o->children, keys );
            children = calloc( *num, sizeof(card*) );
            if ( children != NULL )
            {
                for ( i=0;i<*num;i++ )
                {
                    children[i] = hashmap_get(o->children, keys[i] );
                }
                *success = 1;
            }
            free( keys );
        }
        else
            *num = 0;
    }
    return children;
}
#ifdef MVD_TEST
#define NUM_CHILDREN 3
#define NUM_PARENTS 4
#define NUM_TESTS 5
static pair *pairs[10+NUM_CHILDREN*NUM_PARENTS];
static UChar data[10][10] = { {'b','a','n','a','n','a',0},
{'a','p','p','l','e',0},
{'p','e','a','r',0},
{'o','r','a','n','g','e',0},
{'g','u','a','v','a',0},
{'c','u','m','q','u','a','t',0},
{'p','i','n','e','a','p','p','l','e',0},
{'m','a','n','g','o',0},
{'l','e','m','o','n',0},
{'m','a','n','d','a','r','i','n',0} };
static bitset *bs_random( int cardinality )
{
    bitset *bs = bitset_create();
    int i;
    for ( i=0;i<cardinality;i++ )
    {
        int bit = rand()%16;
        bitset_set( bs, bit );
    }
    return bs;
}
static bitset *bs_random_not( int cardinality, bitset *not )
{
    bitset *bs = bitset_create();
    int i=0;
    while ( i<cardinality )
    {
        int bit = rand()%16;
        if ( bitset_next_set_bit(not,bit) != bit )
        {
            bitset_set( bs, bit );
            i++;
        }
    }
    return bs;
}
/**
 * Generate a random array of pairs. It won't be a valid MVD
 * @return 1 if it worked
 */
static int init_test_data()
{
    int res = 0;
    int id=1;
    srand((int)time(NULL));
    int i,n = sizeof(pairs)/sizeof(pair*);
    int num_data = 10;
    int num_parents = NUM_PARENTS;
    i = 0;
    while ( i<n )
    {
        if ( num_parents > 0 )
        {
            int j,len = u_strlen(data[i]);
            bitset *bs = bs_random(6);
            if ( bs != NULL )
            {
                pair *parent = pair_create_parent(bs,data[i],len);
                if ( parent != NULL )
                {
                    pair_set_id( parent, id++ ); 
                    for ( j=0;j<NUM_CHILDREN;j++ )
                    {
                        bitset *bs2 = bs_random_not(6,bs);
                        if ( bs2 != NULL )
                        {
                            pair *child = pair_create_child(bs2);
                            if ( child != NULL )
                            {
                                pair_set_id( child, id++ ); 
                                parent = pair_add_child( parent, child );
                                if ( parent != NULL )
                                {
                                    pairs[i++] = child;
                                    bitset_dispose( bs2 );
                                }
                                else
                                    break;
                            }
                            else
                                break;
                        }
                        else
                            break;
                    }
                    pairs[i++] = parent;
                    res = (j==NUM_CHILDREN);
                }
                bitset_dispose( bs );
            }
            num_parents--;
        }
        else
        {
            UChar *d = data[rand()%num_data];
            pairs[i++] = pair_create_basic( bs_random(6), d, u_strlen(d) );
            if ( pairs[i-1] == NULL )
                res = 0;
        }
        if ( !res )
            break;
    }
    return res;
}
static void free_test_data()
{
    int i,n = sizeof(pairs)/sizeof(pair*);
    for ( i=0;i<n;i++ )
        if ( pairs[i] != NULL )
            pair_dispose( pairs[i] );
}
static void randomise_pairs()
{
    int i,n = sizeof(pairs)/sizeof(pair*);
    for ( i=0;i<n;i++ )
    {
        int src = rand()%n;
        pair *temp = pairs[src];
        int dst = src;
        while ( dst == src )
            dst = rand()%n;
        pairs[src] = pairs[dst];
        pairs[dst] = temp;
    }
}
static void test_one_orphanage(int *passed, int *failed, plugin_log *log )
{
    randomise_pairs();
    orphanage *o = orphanage_create();
    if ( o != NULL )
    {
        int i,n = sizeof(pairs)/sizeof(pair*);
        for ( i=0;i<n;i++ )
        {
            pair *p = pairs[i];
            card *lp = card_create( p, log );
            if ( pair_is_parent(p) )
                orphanage_add_parent(o,lp);
            else if ( pair_is_child(p) )
                orphanage_add_child(o,lp);
        }
        if ( !orphanage_is_empty(o) )
        {
            fprintf(stderr,"orphanage: not empty after reading entire MVD\n");
            (*failed)++;
        }
        else
            (*passed)++;
        orphanage_dispose( o );
    }
    else
        (*failed)++;
}
void orphanage_test( int *passed, int *failed )
{
    int res = init_test_data();
    if ( res )
    {
        plugin_log *log = plugin_log_create();
        if ( log != NULL )
        {
            int i;
            for ( i=0;i<NUM_TESTS;i++ )
            {
                test_one_orphanage(passed,failed,log);
            }
            plugin_log_dispose( log );
        }
    }
    free_test_data();
}
#endif