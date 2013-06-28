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
static pair *pair_create( bitset*versions, UChar *data, int len, int type )
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
/**
 * Split a pair into two before the given offset. Free the original pair.
 * @param p VAR param the pair to split, becomes leading pair
 * @param at offset into p's data AFTER which to split it
 * @return the new trailing pair or NULL on failure
 */
pair *pair_split( pair **p, int at )
{
    pair *first = pair_create_basic( (*p)->versions, (*p)->data, at+1 );
    pair *second = pair_create_basic( (*p)->versions, &(*p)->data[at+1], 
        (*p)->len-(at+1) );
    if ( first != NULL && second != NULL )
        *p = first;
    else
    {
        if ( first != NULL )
            pair_dispose( first );
        if ( second != NULL )
            pair_dispose( second );
        second = NULL;
    }
    return second;
}
#ifdef DEBUG_PAIR
#include <stdio.h>
int main( int argc, char **argv )
{
    struct basic_pair
    {
        unsigned char *data;
        bitset *versions;
        int len;
    };
    struct child_pair
    {
        pair *parent;
        bitset *versions;
    };
    struct parent_pair
    {
        unsigned char *data;
        int len;
        bitset *versions;
        link_node *children;
    };
    printf("sizeof(struct basic_pair)=%zu sizeof(struct child_pair)=%zu "
        "sizeof(struct parent_pair)=%zu\n",
        sizeof(struct basic_pair),sizeof(struct child_pair),
        sizeof(struct parent_pair));
}
#endif