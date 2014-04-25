#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "pair.h"
#include "encoding.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define DATA_MINSIZE 1
#define BASIC_PAIR 0
#define CHILD_PAIR 1
#define PARENT_PAIR 2
struct pair_struct
{
    bitset *versions;
    union
    {
        link_node *children;
        pair *parent;
    };
    int id;
    short len;
    unsigned char type;
    UChar data[DATA_MINSIZE];
};
/**
 * Private construction method
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length in UChar characters
 * @param type the type of pair
 * @return an allocated pair object or NULL
 */
static pair *pair_create( bitset *versions, UChar *data, int len, int type )
{
    int ulen = len*sizeof(UChar);
    int exists = DATA_MINSIZE*sizeof(UChar);
    size_t extra = (ulen-exists>0)?ulen-exists:0;
    pair *p = calloc( 1, sizeof(pair)+extra );
    if ( p != NULL )
    {
        p->versions = bitset_clone(versions);
        if ( ulen > 0 )
            memcpy( p->data, data, ulen );
        p->type = (unsigned char)type;
        p->len = len;
    }
    else
        fprintf( stderr,"pair: failed to create pair\n");
    return p;
}
/**
 * Create a basic pair. 
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length in UChar characters
 * @return an allocate pair object or NULL
 */
pair *pair_create_basic( bitset *versions, UChar *data, int len )
{
    return pair_create( versions, data, len, BASIC_PAIR );
}
/**
 * Create a hint
 * @param versions the versions of the hint
 * @return the hint
 */
pair *pair_create_hint( bitset *versions )
{
    pair *h = pair_create( versions, NULL, 0, BASIC_PAIR );
    bitset_set( h->versions, 0 );
    return h;
}
/**
 * Get the pair's data. If it is a child, get the parent's data
 * @param p the pair in question
 * @return a UTF-16 encoded string
 */
UChar *pair_data( pair *p )
{
    UChar *data = NULL;
    if ( p->type==CHILD_PAIR )
        data = p->parent->data;
    else
        data = p->data;
    return data;
}
/**
 * Get the pair data's length
 * @param p the pair in question
 * @return the length
 */
int pair_len( pair *p )
{
    if ( p->type==CHILD_PAIR )
        return p->parent->len;
    else
        return p->len;
}
/**
 * Create a child pair (set parent later). 
 * @param versions the versions of this bitset
 * @return an allocate pair object or NULL
 */
pair *pair_create_child( bitset *versions )
{
    pair *p = calloc( 1, sizeof(pair) );
    if ( p != NULL )
    {
        p->versions = bitset_clone(versions);
        p->type = CHILD_PAIR;
    }
    else
        fprintf( stderr,"pair: failed to create child pair\n");
    return p;
}
/**
 * Create a child pair (transpose pair depends on parent). 
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length in UChar characters
 * @return an allocate pair object or NULL
 */
pair *pair_create_parent( bitset *versions, UChar *data, int len )
{
    return pair_create( versions, data, len, PARENT_PAIR );
}
/**
 * Dispose of a single pair
 * @param p the pair in question
 */
void pair_dispose( pair *p )
{
    bitset_dispose( p->versions );
    if ( p->type == PARENT_PAIR && p->children != NULL )
        link_node_dispose( p->children );
    free( p );
}
/**
 * Get the parent of this pair, which may be NULL
 * @param p the pair in question
 * @return the parent
 */
pair *pair_parent( pair *p )
{
    return p->parent;
}
/**
 * Set the parent of this transposed pair
 * @param p the pair to set
 * @param parent its parent
 * @return pointer to the possibly reallocated pair
 */
pair *pair_set_parent( pair *p, pair *parent )
{
    if ( p->type != CHILD_PAIR )
    {
        pair *new = pair_create_child(p->versions);
        // if there was data we lose it here
        pair_dispose( p );
        p = new;
    }
    if ( p != NULL )
        p->parent = parent;
    return p;
}
/**
 * Reset the versions of this pair
 * @param p the pair in question
 * @param v the new set of versions
 */
void pair_set_versions( pair *p, bitset *v )
{
    if ( p->versions != NULL )
        bitset_dispose( p->versions );
    p->versions = v;
}
/**
 * Add a transpose child to this parent
 * @param p the parent to adopt the child
 * @param child the child to adopt
 * @return pointer to the new (or old) pair or NULL if it failed
 */
pair *pair_add_child( pair *p, pair *child )
{
    // get the type right first
    if ( p->type != PARENT_PAIR )
    {
        pair *new = pair_create_parent( p->versions, p->data, p->len );
        pair_dispose( p );
        p = new;
    }
    if ( p != NULL )
    {
        link_node *ln = link_node_create();
        if ( ln != NULL )
        {
            link_node_set_obj( ln, child );
            if ( p->children == NULL )
                p->children = ln;
            else
                link_node_append( p->children, ln );
            pair_set_parent( child, p );
        }
        else
        {
            fprintf(stderr,"pair: failed to add child\n");
            pair_dispose( p );
            p = NULL;
        }
    }
    return p;
}
/**
 * Return the size of the pair itself (minus the data)
 * @param p the pair in question
 * @param versionSetSize the size of a version set in bytes
 * @return the size of the pair when serialised
 */
int pair_size( pair *p, int versionSetSize )
{
    int pSize = versionSetSize + 4 + 4;
    if ( pair_is_parent(p) || pair_is_child(p) )
        pSize += 4;
    return pSize;
}
/**
 * Return the size of the data used by this pair
 * @param p the pair in question
 * @param encoding the destination encoding of the data
 * @return the size of the data only
 */
int pair_datasize( pair *p, char *encoding )
{
    if ( p->type==CHILD_PAIR || pair_is_hint(p) || p->len == 0 )
        return 0;
    else
        return measure_to_encoding( p->data, p->len, encoding );
}
/**
 * Is this pair really a hint?
 * @param p the pair in question
 * @return true if it is, false otherwise
 */
int pair_is_hint( pair *p )
{
    return bitset_get(p->versions,0)==1;
}
int pair_is_ordinary( pair *p )
{
    return p->type==BASIC_PAIR&&!pair_is_hint(p);
}
/**
 * Serialise the versions of the pair
 * @param q the pair in question
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param setSize the size of the versions in bytes
 * @param p the offset within bytes to start writing the versions
 * @return the number of bytes written or 0 on error
 */
static int serialise_versions( pair *q, unsigned char *data, int len, int p, 
    int setSize )
{
    // iterate through the bits
    if ( p+setSize < len )
    {
        int i;
        for ( i=0;i<setSize;i++ ) 
        {
            data[p+i] = bitset_get_byte( q->versions, i );
        }
        return setSize;
    }
    else
    {
        fprintf(stderr,"pair: insufficient space for versions\n");
        return 0;
    }
}
/**
 * Write the pair itself in serialised form but not its data.
 * Versions get written out LSB first.
 * @param p the pair to serialise
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within bytes to start writing this pair
 * @param setSize the number of bytes in the version info
 * @param dataOffset the offset into the data table of this pair's data
 * @param dataLen the length of that data
 * @param parentId the id of the parent or NULL_PID if none
 * @return the number of bytes written to data
 */
int pair_serialise( pair *p, unsigned char *data, int len, int offset, 
    int setSize, int dataOffset, int dataLen, int parentId ) 
{
    int oldOff = offset;
    int flag = 0;
    if ( p->type == CHILD_PAIR )
       flag = CHILD_FLAG;
    else if ( p->type == PARENT_PAIR )
       flag = PARENT_FLAG;
    offset += serialise_versions( p, data, len, offset, setSize );
    // write data offset
    write_int( data, len, offset, dataOffset );
    offset += 4;
    // write data len
    dataLen |= flag;
    write_int( data, len, offset, dataLen );
    offset += 4;
    if ( parentId != NULL_PID )
    {
        write_int( data, len, offset, parentId );
        offset += 4;
    }
    return offset - oldOff;
}
/**
 * Write the data for the pair
 * @param p the pair whose data is to be serialised
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param dataTableOffset offset of the data table
 * @param dataOffset offset into data table of this pair's data
 * @param encoding the encoding of the data
 * @return number of bytes written
 */
int pair_serialise_data( pair *p, unsigned char *data, int len, 
    int dataTableOffset, int dataOffset, char *encoding )
{
    int nchars=0;
    if ( p->type != CHILD_PAIR && !pair_is_hint(p) && p->len > 0 )
    {
        int offset = dataTableOffset+dataOffset;
        nchars = convert_to_encoding( p->data, p->len, 
            &data[offset], len-offset, encoding );
        if ( nchars == 0 )
            fprintf(stderr,"pair: failed to encode pair data\n");
    }
    return nchars;
}
int pair_equals( pair *p, pair *q, char *encoding )
{
    if ( p->type!=q->type )
        return 0;
    else if ( p->len != q->len )
        return 0;
    else if ( bitset_get(p->versions,0)!=bitset_get(q->versions,0))
        return 0;
    else if ( p->len > 0 )
    {
        int len1 = measure_to_encoding(p->data,p->len,encoding);
        int len2 = measure_to_encoding(q->data,q->len,encoding);
        if ( len1!=len2 )
            return 0;
    }
    return 1;
}
/**
 * Is this pair a child of something?
 * @param p the pair in question
 * @return 1 if it is else 0
 */
int pair_is_child( pair *p )
{
    return p->type == CHILD_PAIR;
}
int pair_set_id( pair *p, int id )
{
    p->id = id;
    return id;
}
int pair_id( pair *p )
{
    return p->id;
}
int pair_is_parent( pair *p )
{
    return p->type == PARENT_PAIR;
}
link_node *pair_first_child( pair *p )
{
    return p->children;
}
bitset *pair_versions( pair *p )
{
    return p->versions;
}
static char *pair_cdata( pair *p )
{
    int i;
    char *buf = calloc( p->len+1, 1 );
    if ( buf != NULL )
    {
        for ( i=0;i<p->len;i++ )
            buf[i] = (char)p->data[i];
        buf[i] = 0;
    }
    return buf;
}
void pair_print( pair *p )
{
    if ( pair_is_parent(p) )
    {
        char buf[128];
        bitset_serialise(pair_versions(p),buf,128 );
        printf(", versions: %s",buf);
        char *pdata = pair_cdata(p);
        if ( pdata != NULL )
        {
            printf(", parent: %s, id: %d\n",pdata,pair_id(p));
            free( pdata );
        }
    }
    else if ( pair_is_child(p) )
        printf(", child: parent=%d\n",pair_id(pair_parent(p)));
    else
    {
        char buf[128];
        bitset_serialise(pair_versions(p),buf,128 );
        printf(", versions: %s",buf);
        char *pdata = pair_cdata(p);
        if ( pdata != NULL )
        {
            printf(", data: %s\n",pdata);
            free( pdata );
        }
    };
}
/**
 * Split a pair into two before the given offset. Free the original pair.
 * @param p VAR param the pair to split, becomes leading pair
 * @param at offset into p's data BEFORE which to split it
 * @return the new trailing pair or NULL on failure
 */
pair *pair_split( pair **p, int at )
{
    int old_type = (*p)->type;
    if ( old_type == PARENT_PAIR )
        printf("PARENT\n");
    pair *first = pair_create_basic( (*p)->versions, (*p)->data, at );
    pair *second = pair_create_basic( (*p)->versions, &(*p)->data[at], 
        (*p)->len-at );
    pair_dispose( *p );
    if ( first != NULL && second != NULL )
    {
        *p = first;
        first->type = second->type = old_type;
    }
    else
    {
        if ( first != NULL )
            pair_dispose( first );
        if ( second != NULL )
            pair_dispose( second );
        second = NULL;
        *p = NULL;
    }
    return second;
}
#ifdef MVD_TEST
static UChar data1[] = {'b','a','n','a','n','a'};
static UChar data2[] = {'a','p','p','l','e'}; 
static void test_pair_parent( int *passed, int *failed )
{
    int nbytes2 = sizeof(data2)/sizeof(UChar);
    bitset *bs2 = bitset_create();
    if ( bs2 != NULL )
        bs2 = bitset_set( bs2, 5 );
    bitset *bs3 = bitset_create();
    if ( bs3 != NULL )
        bs3 = bitset_set( bs3, 14 );
    if ( bs2 != NULL && bs3 != NULL )
    {
        pair *p2 = pair_create_parent( bs2, (UChar*)data2, nbytes2);
        pair *p3 = pair_create_child( bs3 );
        p2 = pair_add_child( p2, p3 );
        p3 = pair_set_parent( p3, p2 );
        if ( pair_parent(p3) != p2 )
        {
            (*failed)++;
            fprintf(stderr,"pair: failed to add child\n");
        }
        else
            (*passed)++;
        pair_dispose( p2 );
        pair_dispose( p3 );
        bs2 = bitset_create();
        bs3 = bitset_create();
        if ( bs2 != NULL )
            bs2 = bitset_set( bs2, 7 );
        if ( bs3 != NULL )
            bs3 = bitset_set( bs3, 23 );
        if ( bs2 != NULL && bs3 != NULL )
        {
            p2 = pair_create_parent( bs2, data2, nbytes2 );
            p3 = pair_create_child( bs3 );
            if ( p2 != NULL && p3 != NULL )
            {
                p2 = pair_add_child(p2,p3);
                p3 = pair_set_parent( p3, p2 );
                if ( p2 != NULL && p3 != NULL )
                {
                    if ( pair_parent(p3)!= p2 )
                    {
                        fprintf(stderr,"pair: failed to set parent\n");
                        (*failed)++;
                    }
                    else
                        (*passed)++;
                    link_node *ln = pair_first_child(p2);
                    if ( ln==NULL|| link_node_obj(ln)!=p3 )
                    {
                        fprintf(stderr,"pair: failed to set child\n");
                        (*failed)++;
                    }
                    else
                        (*passed)++;
                }
                else
                {
                    fprintf(stderr,"pair: failed to set parent or child\n");
                    (*failed) += 2;
                }
            }
            else
                (*failed)++;
        }
        if ( pair_len(p2)!=pair_len(p3)|| pair_len(p2)!=nbytes2 )
        {
            fprintf(stderr,"pair: pair length incorrect\n");
            (*failed)++;
        }
        else
            (*passed)++;
        if ( p2 != NULL )
            pair_dispose( p2 );
        if ( p3 != NULL )
            pair_dispose( p3 );
    }
    else
        (*failed)++;
}
static void test_pair_hint( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    if ( bs != NULL )
        bs = bitset_set( bs, 7 );
    if ( bs != NULL )
        bs = bitset_set( bs, 23 );
    pair *p = pair_create_hint( bs );
    if ( p != NULL )
    {
        if ( !pair_is_hint(p)||pair_is_ordinary(p)
            ||pair_is_parent(p)||pair_is_child(p) )
        {
            fprintf(stderr,"pair: failed to create valid hint\n");
            (*failed)++;
        }
        else
            (*passed)++;
        if ( pair_len(p)!=0 )
        {
            fprintf(stderr,"pair: failed to create hint\n");
            (*failed)++;
        }
        else
            (*passed)++;
        pair_dispose( p );
    }
    else
        (*failed)++;
}
static void test_pair_versions( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    if ( bs != NULL )
        bs = bitset_set( bs, 7 );
    if ( bs != NULL )
        bs = bitset_set( bs, 23 );
    int nbytes1 = sizeof(data1)/sizeof(UChar);
    pair *p = pair_create_basic( bs,data1, nbytes1 );
    if ( p != NULL )
    {
        bitset *bs2 = bitset_create();
        if ( bs2 != NULL )
        {
            bs2 = bitset_set( bs2, 13 );
            if ( bs2 != NULL )
            {
                pair_set_versions( p, bs2 );
                bitset *pv = pair_versions(p);
                if ( bitset_next_set_bit(pv,13)!=13 
                    || bitset_next_set_bit(pv,23)==23 )
                {
                    fprintf(stderr,"pair: failed to reset versions\n");
                    (*failed)++;
                }
                else
                    (*passed)++;
            }
            else
                (*failed)++;
        }
        pair_dispose( p );
    }
    else
        (*failed)++;
}
static void test_pair_id( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    if ( bs != NULL )
        bs = bitset_set( bs, 7 );
    if ( bs != NULL )
        bs = bitset_set( bs, 23 );
    int nbytes1 = sizeof(data1)/sizeof(UChar);
    if ( bs != NULL )
    {
        pair *p = pair_create_basic( bs, data1, nbytes1 );
        if ( p != NULL )
        {
            pair_set_id( p, 33 );
            if ( pair_id(p) != 33 )
            {
                fprintf(stderr,"pair: failed to set id\n");
                (*failed)++;
            }
            else
                (*passed)++;
            pair_dispose( p );
        }
        else
            (*failed)++;
    }
    else
        (*failed)++;
}
static void test_pair_split( int *passed, int *failed )
{
    bitset *bs = bitset_create();
    if ( bs != NULL )
        bs = bitset_set( bs, 7 );
    if ( bs != NULL )
        bs = bitset_set( bs, 23 );
    int nbytes1 = sizeof(data1)/sizeof(UChar);
    if ( bs != NULL )
    {
        pair *p = pair_create_basic( bs, data1, nbytes1 );
        if ( p != NULL )
        {
            int at = nbytes1/2;
            pair *q = pair_split( &p, at );
            if ( q != NULL )
            {
                if ( pair_len(p)!= at || pair_len(q) != nbytes1-at )
                {
                    fprintf(stderr,"pair: failed to split\n");
                    (*failed)++;
                }
                else
                    (*passed)++;
            }
            else
            {
                fprintf(stderr,"pair: failed to split\n");
                (*failed)++;
            }
        }
        pair_dispose( p );
    }
}
void test_pair( int *passed, int *failed )
{
    test_pair_parent( passed, failed );
    test_pair_hint( passed, failed );
    test_pair_versions( passed, failed );
    test_pair_id( passed, failed );
    test_pair_split( passed, failed );
}
#endif