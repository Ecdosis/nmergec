#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitset.h"
#include "link_node.h"
#include "mvd/pair.h"

// assuming we are on an 8 byte system
#define DATA_MINSIZE 1
#define BASIC_PAIR 0
#define CHILD_PAIR 1
#define PARENT_PAIR 2
// this should usually be at least of size 24 
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
    unsigned char data[DATA_MINSIZE];
};
/**
 * Create a basic pair. 
 * @param versions the versions of this bitset
 * @param data the data of the pair
 * @param len the data length
 * @return an allocate pair object or NULL
 */
pair *pair_create_basic( bitset *versions, unsigned char *data, int len )
{
    size_t extraDataSize = (len-DATA_MINSIZE>0)?len-DATA_MINSIZE:0;
    pair *p = calloc( 1, sizeof(pair)+extraDataSize );
    if ( p != NULL )
    {
        p->versions = bitset_clone(versions);
        memcpy( p->data, data, len );
        p->type = BASIC_PAIR;
        p->len = len;
    }
    else
        fprintf( stderr,"pair: failed to create basic pair\n");
    return p;
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
 * @param len the data length
 * @return an allocate pair object or NULL
 */
pair *pair_create_parent( bitset *versions, unsigned char *data, int len )
{
    size_t extraDataSize = (len-DATA_MINSIZE>0)?len-DATA_MINSIZE:0;
    pair *p = calloc( 1, sizeof(pair)+extraDataSize );
    if ( p != NULL )
    {
        p->versions = bitset_clone(versions);
        memcpy( p->data, data, len );
        p->type = PARENT_PAIR;
        p->len = len;
    }
    else
        fprintf( stderr,"pair: failed to create parent pair\n");
    return p;
}
/**
 * Dispose of a single pair
 * @param p the pair in question
 */
void pair_dispose( pair *p )
{
    bitset_dispose( p->versions );
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
* @return the size of the data only
*/
int pair_datasize( pair *p )
{
    if ( p->parent!=NULL || pair_is_hint(p) )
        return 0;
    else if ( p->data == NULL )
        return 0;
    else
        return p->len;
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
        for ( i=bitset_get(q->versions,0); i>=0; 
            i=bitset_next_set_bit(q->versions,i+1) ) 
        {
            int index = ((setSize*8-1)-i)/8;
            if ( index < 0 )
            {    
                fprintf(stderr,"pair: serialising versions: byte index < 0\n");
                break;
            }
            data[p+index] |= 1 << (i%8);
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
 * Write the pair itself in serialised form and also its data.
 * Versions get written out LSB first.
 * @param p the pair to serialise
 * @param data the byte array to write to
 * @param len the overall length of data
 * @param p the offset within bytes to start writing this pair
 * @param setSize the number of bytes in the version info
 * @param dataOffset offset reserved in the dataTable for this 
 * @param dataTableOffset the start of the data table within data
 * pair's data (might be the same as some other pair's)
 * @param parentId the id of the parent or NULL_PID if none
 * @return the number of bytes written to data
 */
int pair_serialise( pair *p, unsigned char *data, int len, int offset, 
    int setSize, int dataOffset, int dataTableOffset, int parentId ) 
{
    int oldOff = offset;
    int flag = 0;
    if ( p->type == CHILD_PAIR )
       flag = CHILD_FLAG;
    else if ( p->type == PARENT_PAIR )
       flag = PARENT_FLAG;
    offset += serialise_versions( p, data, len, offset, setSize );
    // write data offset
    // can't see the point of this any more
    //writeInt( bytes, p, (children==null)?dataOffset:-dataOffset );
    write_int( data, len, offset, dataOffset );
    offset += 4;
    // write data length ORed with the parent/child flag
    int dataLength = (p->data==NULL)?0:p->len; 
    dataLength |= flag;
    write_int( data, len, offset, dataLength );
    offset += 4;
    if ( parentId != NULL_PID )
    {
        write_int( data, len, offset, parentId );
        offset += 4;
    }
    // write actual data
    if ( p->parent == NULL && !pair_is_hint(p) )
        offset += write_data( data, len, dataTableOffset+dataOffset, 
            p->data, p->len );
    return offset - oldOff;
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