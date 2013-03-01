#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "pair.h"
#include "version.h"
#include "mvd.h"
#include "plugin.h"
#include "encoding.h"
#include "plugin_log.h"
#include "suffixtree.h"
#include "hashmap.h"
#include "utils.h"
#include "option_keys.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

// the version ID - a /-delimited string
static UChar *vid=NULL;
// description of the version to be added
static UChar *v_description=NULL;
// the text itself - we use wide UTF-16 as the basic char type
static UChar *text;
// number of wchar_t characters in text 
static size_t tlen;
// encoding of the input file
static char *encoding="utf-8";

/**
 * Convert an ascii string to unicode using the given encoding
 * @param str an 8-bit plain string
 * @param encoding the encoding's canonical name
 * @param log a plugin log to record errors
 * @return an allocated UChar string or NULL
 */
static UChar *ascii_to_unicode( char *str, char *encoding, plugin_log *log )
{
    int u_len = measure_from_encoding( str, strlen(str), encoding );
    UChar *dst = calloc( u_len+1, sizeof(UChar) );
    if ( dst != NULL )
    {
        int nchars = convert_from_encoding( str,strlen(str),
            dst,u_len+1,encoding);
        if ( nchars == 0 )
        {
            plugin_log_add(log,
                "mvd_add: failed to convert %s via encoding %s\n",
                str,encoding);
            free( dst );
            dst = NULL;
        }
    }
    else
        plugin_log_add(log,"mvd_add: failed to allocate UChar buffer\n");
    return dst;
}
/**
 * Parse the options
 * @param map of key-value pairs already parsed
 * @param log the log to record errors in
 * @return 1 if the options were sane
 */
static int set_options( hashmap *map, plugin_log *log )
{
    int sane = 1;
    if ( hashmap_contains(map,ENCODING_KEY) )
        encoding = hashmap_get(map,ENCODING_KEY);
    if ( hashmap_contains(map,VID_KEY) )
    {
        vid = ascii_to_unicode( hashmap_get(map,VID_KEY), encoding, log );
        if ( vid == NULL )
            sane = 0;
    }
    else
    {
        plugin_log_add(log,"mvd_add: missing required version ID\n");
        sane = 0;
    }
    if ( hashmap_contains(map,DESCRIPTION_KEY) )
    {
        v_description = ascii_to_unicode(hashmap_get(map,DESCRIPTION_KEY),
            encoding,log );
        if ( v_description == NULL )
            sane = 0;
    }
    return sane;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulate or read
 * @param options a string containing the plugin's options
 * @param output buffer of length SCRATCH_LEN
 * @param data data passed directly in 
 * @param data_len length of data
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char *output, 
    unsigned char *data, size_t data_len )
{
    int res = 0;
    plugin_log *log = plugin_log_create( output );
    if ( log != NULL )
    {
        hashmap *map = parse_options( options );
        if ( map != NULL )
        {
            res = set_options( map, log );
            if ( res && data != NULL && data_len > 0 )
            {
                char *mvd_encoding = mvd_get_encoding(*mvd);
                printf("mvd_encoding=%s\n",mvd_encoding);
                if ( strcmp(mvd_encoding,encoding)!=0 )
                {
                    plugin_log_add(log,
                        "file's encoding %s does not match mvd's (%s):"
                        " assimilating...\n",encoding,mvd_encoding);
                }
                int nbytes = measure_from_encoding( 
                    (char*)data, data_len, encoding );
                text = malloc( nbytes+sizeof(UChar) );
                if ( text != NULL )
                {
                    nbytes = convert_from_encoding(data, data_len, text, 
                        nbytes/sizeof(UChar)+1, encoding)/sizeof(UChar);
                    if ( nbytes > 0 )
                    {
                        tlen = nbytes/sizeof(UChar);
                        suffixtree *st = suffixtree_create( text, tlen, log );
                        // then process the suffix tree...
                        if ( st != NULL )
                        {
                            plugin_log_add( log, "created tree successfully!\n");
                            suffixtree_dispose( st );
                        }
                    }
                    else
                        res = 0;
                }
                else
                {
                    plugin_log_add(log,"failed to allocate uchar buffer\n");
                    res = 0;
                }
            }
        }
    }
    return res;
}
/**
 * Print a help message to stdout explaining what the parameters are
 */
char *help()
{
    return "options:\n\t"
        "text=<file>: file to add to existing MVD (optional)\n\t"
        "vid=<vid>: version ID as a /-delimited path\n\t"
        "encoding=<ascii-encoding-name>: the text encoding\n\t"
        "description=<string>: description of new version in encoding\n\t"
        "debug=1|0: turn debugging on (default off)\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    return "version 1.0 (c) 2013 Desmond Schmidt\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "add";
}
/**
 * Provide a description of this plugin
 * @return a string
 */
char *description()
{
    return "adds a new version to an existing mvd\n";
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test( int *p,int *f )
{
    return 1;
}
