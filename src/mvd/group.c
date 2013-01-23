#include <stdio.h>
#include <stdlib.h>
#include "mvd/group.h"
/** Stores group information for old-format MVDs */
struct group_struct
{
    int id;
    int parent;
};

/**
 * Create a group. Parent can be updated later
 * @param id the id of the group
 * @param parent the parent or 0
 * @return the finished group or NULL on failure
 */
group *group_create( int id, int parent )
{
    group *g = calloc( 1, sizeof(group) );
    if ( g != NULL )
    {
        g->id = id;
        g->parent = parent;
    }
    else
        fprintf(stderr,"group: failed to create group\n");
    return g;
}
/**
 * Dispose of a group. Simple.
 * @param g the group to dispose of
 */
void group_dispose( group *g )
{
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