#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mvd/version.h"
struct version_struct
{
    char *id;
    char *description;
};
/**
 * Create a version
 * @param id the path to that version e.g. Base/F1
 * @param description the long name or description of the version
 * @return the competed version obejct or NULL on failure
 */
version *version_create( char *id, char *description )
{
    version *v = calloc( 1, sizeof(version) );
    if ( v != NULL )
    {
        v->id = strdup( id );
        v->description = strdup( description );
        if ( v->id == NULL || v->description == NULL )
        {
            version_dispose( v );
            v = NULL;
        }
    }
    else
    {
        fprintf(stderr,"version: failed to allocate object\n");
    }
    return v; 
}
/**
 * Dispose of a version object
 * @param v the version in question
 */
void version_dispose( version *v )
{
    if ( v->id != NULL )
        free( v->id );
    if ( v->description != NULL )
        free( v->description );
    free( v );
}
/**
 * Get a version's description 
 * @param v the version in question
 * @return the description
 */
char *version_description( version *v )
{
    return v->description;
}
/**
 * Get a version's id 
 * @param v the version in question
 * @return the id
 */
char *version_id( version *v )
{
    return v->id;
}
/**
 * Compute the amount of store the version requires on serialisation
 * @param v the version in question
 * @return the serialised length
 */
int version_datasize( version *v, int old )
{
    int nBytes = 4+strlen(v->description);
    char *slash_pos = strrchr(v->id,'/');
    if ( old )
        nBytes += 2;    // add gid
    if ( slash_pos == NULL || !old )
        return nBytes+strlen(v->id);
    else
        return nBytes+strlen(slash_pos)-1;
}