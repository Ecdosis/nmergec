#include <stdlib.h>
#include <stdio.h>
#include "unicode/uchar.h"
#include "version.h"
#include "hashmap.h"
#include "utils.h"
#include "unicode/ustring.h"
#include "encoding.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
struct version_struct
{
    UChar *id;
    UChar *description;
};
/**
 * Create a version
 * @param id the path to that version e.g. Base/F1
 * @param description the long name or description of the version
 * @return the competed version object or NULL on failure
 */
version *version_create( UChar *id, UChar *description )
{
    version *v = calloc( 1, sizeof(version) );
    if ( v != NULL )
    {
        v->id = u_strdup( id );
        v->description = u_strdup( description );
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
UChar *version_description( version *v )
{
    return v->description;
}
/**
 * Get a version's id 
 * @param v the version in question
 * @return the id
 */
UChar *version_id( version *v )
{
    return v->id;
}
/**
 * Compute the amount of store the version requires on serialisation
 * @param v the version in question
 * @param old 1 if using the old MVD format
 * @param encoding the encoding of the version data
 * @return the serialised length
 */
int version_datasize( version *v, int old, char *encoding )
{
    int nbytes = 4+measure_to_encoding( v->description, 
        u_strlen(v->description), encoding );
    UChar *slash_pos = u_strrchr( v->id, (UChar)0x2F );
    if ( old )
        nbytes += 2;    // add gid
    if ( slash_pos == NULL || !old )
        return nbytes+measure_to_encoding(v->id,u_strlen(v->id),encoding);
    else
        return nbytes+measure_to_encoding(slash_pos+1,u_strlen(slash_pos+1),
            encoding);
}