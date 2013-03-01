#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/uchar.h"
#include "group.h"
#include "hashmap.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/** Stores group information for old-format MVDs */
struct group_struct
{
    int id;
    int parent;
    int size;
    UChar *name;
};

/**
 * Create a group. Parent can be updated later
 * @param id the id of the group
 * @param parent the parent or 0
 * @param name the group's name
 * @return the finished group or NULL on failure
 */
group *group_create( int id, int parent, UChar *name )
{
    group *g = calloc( 1, sizeof(group) );
    if ( g != NULL )
    {
        g->id = id;
        g->parent = parent;
        g->name = u_strdup( name );
        if ( name == NULL )
        {
            group_dispose( g );
            g = NULL;
            fprintf(stderr,"group: failed to duplicate name\n");
        }
    }
    else
        fprintf(stderr,"group: failed to create group\n");
    return g;
}
/**
 * Dispose of a group. Simple.
 * @param g the group to dispose of (must be void so it can be 
 * disposed of via hashmap)
 */
void group_dispose( void *g )
{
    if ( ((group*)g)->name != NULL )
        free( ((group*)g)->name );
    free( g );
}
/**
 * Set the parent group
 * g the group in question
 * @param parent its new parent
 */
void group_set_parent( group *g, int parent )
{
    g->parent = parent;
}
/**
 * Get this group's ID
 * @param g the group
 * @return its ID (always correct)
 */
int group_id( group *g )
{
    return g->id;
}
/**
 * Get this group's parent id
 * @param g the gorup in question
 * @return a parent id
 */
int group_parent( group *g )
{
    return g->parent;
}
/**
 * Return the size of this Group object for serialization
 * @param g the group to measure
 * @param encoding its destination encoding
 * @return the size in bytes
 */
int group_datasize( group *g, char *encoding )
{
   if ( g->size == 0 )
   {
       int name_len = measure_to_encoding( g->name, u_strlen(g->name), 
           encoding );
       g->size = 2 + 2 + name_len;
   }
   return g->size;
}