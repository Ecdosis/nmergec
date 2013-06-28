#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "dyn_string.h"
#include "hashmap.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define DYN_DEFAULT_LEN 128
struct dyn_string_struct
{
    int len;// length in UChars
    int allocated;// size in UChars
    UChar *data;
};
/**
 * Create a dynamic string object
 * @return an allocated basic dynamic string or NULL
 */
dyn_string *dyn_string_create()
{
    dyn_string *ds = calloc( 1, sizeof(dyn_string) );
    if ( ds != NULL )
    {
        ds->data = calloc( DYN_DEFAULT_LEN, sizeof(UChar) );
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
 * Create a dynamic string object from a fixed string
 * @param str an existing fixed-length string
 * @return an allocated basic dynamic string or NULL
 */
dyn_string *dyn_string_create_from( UChar *str )
{
    dyn_string *ds = dyn_string_create();
    if ( !dyn_string_concat(ds,str) )
    {
        dyn_string_dispose( ds );
        ds = NULL;
    }
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
    free( ds );
    return NULL;
}
/**
 * Reallocate this string buffer
 * @param ds the object reference
 * @param wanted we need at least this many UChars overall
 * @return 1 if it worked
 */
static int dyn_string_resize( dyn_string *ds, int wanted )
{
    int res = 1;
    int inc = wanted + wanted%DYN_DEFAULT_LEN + DYN_DEFAULT_LEN;
    UChar *temp = calloc( inc, sizeof(UChar) );
    if ( temp != NULL )
    {
        u_strcpy( temp, ds->data );
        ds->allocated = inc;
        free( ds->data );
        ds->data = temp;
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
int dyn_string_concat( dyn_string *ds, UChar *tail )
{
    int res = 1;
    int tlen = u_strlen(tail);
    if ( ds->len+tlen+1 > ds->allocated )
        res = dyn_string_resize(ds,ds->len+tlen+1);
    if ( res )
    {
        u_strcat( ds->data, tail );
        ds->len += tlen;
    }
    return res;
}
/**
 * Get the length of a dynamic string
 * @param ds the dynamic string object
 * @return its length
 */
int dyn_string_len( dyn_string *ds )
{
    return ds->len;
}
/**
 * Get a dynamic string's actual data
 * @param ds the object in question
 * @return its actual string data
 */
UChar *dyn_string_data( dyn_string *ds )
{
    return ds->data;
}
/**
 * Test the dyn_string object
 * @param passed VAR param update number of passed tests
 * @param failed VAR param update number of failed tests
 */
void test_dyn_string( int *passed, int *failed )
{
    int res = 0;
    dyn_string *ds = dyn_string_create();
    if ( ds != NULL )
    {
        res = dyn_string_concat( ds, (UChar*)
            "\x0048\x0065\x006C\x006C\x006F\x0020" );
        if ( res )
            res = dyn_string_concat( ds, 
                (UChar*)"\x004B\x0069\x0074\x0074\x0079\x002E\x0020" );
        if ( res )
            res = dyn_string_concat( ds, (UChar*)"\x004C\x006F\x0072\x0065"
            "\x006D\x0020\x0069\x0070\x0073\x0075\x006D\x0020\x0064\x006F"
            "\x006C\x006F\x0072\x0020\x0073\x0069\x0074\x0020\x0061\x006D"
            "\x0065\x0074\x002C\x0020\x0063\x006F\x006E\x0073\x0065\x0063"
            "\x0074\x0065\x0074\x0075\x0072\x0020\x0061\x0064\x0069\x0070"
            "\x0069\x0073\x0069\x0063\x0069\x006E\x0067\x0020\x0065\x006C"
            "\x0069\x0074\x002C\x0020\x0073\x0065\x0064\x0020\x0064\x006F"
            "\x0020\x0065\x0069\x0075\x0073\x006D\x006F\x0064\x0020\x0074"
            "\x0065\x006D\x0070\x006F\x0072\x0020\x0069\x006E\x0063\x0069"
            "\x0064\x0069\x0064\x0075\x006E\x0074\x0020\x0075\x0074\x0020"
            "\x006C\x0061\x0062\x006F\x0072\x0065\x0020\x0065\x0074\x0020"
            "\x0064\x006F\x006C\x006F\x0072\x0065\x0020\x006D\x0061\x0067"
            "\x006E\x0061\x0020\x0061\x006C\x0069\x0071\x0075\x0061\x002E" );
        //printf("res=%d len=%d data=%s\n",res,ds->len,ds->data);
        dyn_string_dispose( ds );
    }
    if ( res )
        *passed += 1;
    else
        *failed += 1;
}