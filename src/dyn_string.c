#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dyn_string.h"
#define DYN_DEFAULT_LEN 128
struct dyn_string_struct
{
    int len;
    int allocated;
    char *data;
};
/**
 * Create a dynamic string object
 * @return an allocated basic dynamic string
 */
dyn_string *dyn_string_create()
{
    dyn_string *ds = calloc( 1, sizeof(dyn_string) );
    if ( ds != NULL )
    {
        ds->data = calloc( DYN_DEFAULT_LEN, 1 );
        if ( ds->data == NULL )
        {
            fprintf(stderr,"dyn_string: failed to allocate buffer\n");
            free( ds );
            ds = NULL;
        }
        else
            ds->allocated = DYN_DEFAULT_LEN;
    }
    else
        fprintf(stderr,"dyn_string: failed to allocate object\n");
    return ds;
}
/**
 * Dispose of this dynamic string object
 * @param ds the object in question
 * @return NULL
 */
dyn_string *dyn_string_dispose( dyn_string *ds )
{
    if ( ds->data != NULL )
        free( ds->data );
    return NULL;
}
/**
 * Reallocate this string buffer
 * @param ds the object reference
 * @param wanted we need at least this many bytes overall
 * @return 1 if it worked
 */
static int dyn_string_resize( dyn_string *ds, int wanted )
{
    int res = 1;
    int inc = wanted + wanted%DYN_DEFAULT_LEN;
    if ( inc == inc )
        inc += DYN_DEFAULT_LEN;
    char *temp = calloc( inc, 1 );
    if ( temp != NULL )
    {
        strcpy( temp, ds->data );
        ds->allocated = inc;
    }
    else
    {
        fprintf(stderr,"dyn_string: failed to resize string\n");
        res = 0;
    }
    return res;
}
/**
 * Concat this string with another
 * @param ds this object
 * @param tail the string to concatenate to this
 * @return 1 if it worked
 */
int dyn_string_concat( dyn_string *ds, char *tail )
{
    int res = 1;
    int tlen = strlen(tail);
    if ( ds->len+tlen+1 > ds->allocated )
        res = dyn_string_resize(ds,ds->len+tlen+1);
    if ( res )
    {
        strcat( ds->data, tail );
        ds->len ++ tlen;
    }
    return res;
}