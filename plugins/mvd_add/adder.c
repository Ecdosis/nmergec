#include <stdlib.h>
#include <string.h>
#include <unicode/uchar.h>
#include "hashmap.h"
#include "plugin_log.h"
#include "adder.h"
#include "option_keys.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/*
 * Store information about a version to be added to the MVD
 */
struct adder_struct
{
    // the version ID - a /-delimited string
    UChar *vid;
    // description of the version to be added
    UChar *v_description;
    // MVD description - if need to create one
    UChar *mvd_description;
    // the text itself - we use wide UTF-16 as the basic char type
    UChar *text;
    // number of wchar_t characters in text 
    size_t tlen;
    // encoding of the input file
    char *encoding;
    // options as key/value pairs
    hashmap *options;
    // log to record error in
    plugin_log *log;
};

/**
 * Create an adder
 * @return the adder instance
 */
adder *adder_create( plugin_log *log )
{
    adder *a = calloc( 1, sizeof(adder) );
    if ( a != NULL )
        a->log = log;
    else
        plugin_log_add( log, "adder: failed to allocate object\n");
}
/**
 * Dispose of this object
 * @param add the object to dispose
 */
void adder_dispose( adder *add )
{
    if ( add->mvd_description != NULL )
        free( add->mvd_description );
    if ( add->v_description != NULL )
        free( add->v_description );
    if ( add->vid != NULL )
        free( add->vid );
    free( add );
}
/**
 * Convert a string to UTF-16 from its own encoding
 * @param src an 8-bit plain string
 * @param src_len the length of src in BYTES
 * @param encoding the src encoding's canonical name
 * @param dst_len VAR param to whole number of UChars converted into
 * @param log a plugin log to record errors
 * @return an allocated UChar string or NULL
 */
UChar *convert_to_unicode( char *src, int src_len, char *encoding, 
    int *dst_len, plugin_log *log )
{
    int u_len = measure_from_encoding( src, src_len, encoding );
    UChar *dst = calloc( u_len+1, sizeof(UChar) );
    if ( dst != NULL )
    {
        int nchars = convert_from_encoding( src, src_len,
            dst,u_len+1,encoding);
        if ( nchars == 0 )
        {
            plugin_log_add(log,
                "mvd_add: failed to convert %s via encoding %s\n",
                src, encoding);
            free( dst );
            dst = NULL;
        }
        else
            *dst_len = nchars/sizeof(UChar);
    }
    else
        plugin_log_add(log,"mvd_add: failed to allocate UChar buffer\n");
    return dst;
}
/**
 * Parse the options
 * @param add the adder instance
 * @param map of key-value pairs already parsed
 * @return 1 if the options were sane
 */
int adder_set_options( adder *add, hashmap *map )
{
    int sane = 1;
    char *value;
    if ( hashmap_contains(map,ENCODING_KEY) )
        add->encoding = hashmap_get(map,ENCODING_KEY);
    if ( hashmap_contains(map,VID_KEY) )
    {
        int tlen;
        value = hashmap_get(map,VID_KEY);
        add->vid = convert_to_unicode( value, strlen(value), add->encoding, 
            &tlen, add->log );
        if ( add->vid == NULL )
            sane = 0;
    }
    else
    {
        plugin_log_add(add->log,"mvd_add: missing version ID\n");
        sane = 0;
    }
    if ( hashmap_contains(map,LONG_NAME_KEY) )
    {
        value = hashmap_get(map,LONG_NAME_KEY);
        int tlen;
        add->v_description = convert_to_unicode(value,strlen(value),
            add->encoding,&tlen,add->log );
        if ( add->v_description == NULL )
        {
            sane = 0;
            plugin_log_add(add->log,"mvd_add: missing version description\n");
        }
    }
    else if ( add->vid != NULL )
    {
        add->v_description = u_strdup(DEFAULT_LONG_NAME);
    }
    if ( hashmap_contains(map,DESCRIPTION_KEY) )
    {
        value = hashmap_get(map,DESCRIPTION_KEY);
        int tlen;
        adder_set_mvd_desc(add, convert_to_unicode(value,strlen(value),
            add->encoding,&tlen,add->log) );
        if ( add->mvd_description == NULL )
        {
            sane = 0;
            plugin_log_add(add->log,"mvd_add: missing mvd description\n");
        }
    }
    else
        adder_set_mvd_desc(add, u_strdup(DEFAULT_DESCRIPTION) );
    return sane;
}
UChar *adder_vid( adder *a )
{
    return a->vid;
}
UChar *adder_version_desc( adder *a )
{
    return a->v_description;
}
void adder_set_mvd_desc( adder *a, UChar *desc )
{
    a->mvd_description = desc;
}
UChar *adder_mvd_desc( adder *a )
{
    return a->mvd_description;
}
char *adder_encoding( adder *a )
{
    return a->encoding;
}