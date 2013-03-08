#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "version.h"
#include "pair.h"
#include "mvd.h"
#include "hashmap.h"
#include "plugin.h"
#include "utils.h"
#include "encoding.h"
#include "plugin_log.h"
#include "utils.h"
#include "option_keys.h"

static int debug = 0;

/**
 * Do the work of this plugin
 * @param mvd VAR param the mvd to create is stored here
 * @param options a string contianing the plugin's options
 * @param output buffer of length SCRATCH_LEN
 * @param data ignored
 * @param data_len ignored
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char *scratch, 
unsigned char *data, size_t data_len )
{
    int res = 1;
    plugin_log *log = plugin_log_create( scratch );
    *mvd = mvd_create();
    if ( *mvd != NULL )
    {
        hashmap *map = parse_options( options );
        if ( map != NULL )
        {
            char *encoding = hashmap_get( map, ENCODING_KEY );
            char *description = hashmap_get( map, DESCRIPTION_KEY );
            if ( encoding == NULL )
                encoding = "utf-8";
            else
                lowercase( encoding );
            if ( hashmap_contains(map,DEBUG_KEY) )
                debug = atoi(hashmap_get(map,DEBUG_KEY));
            if ( debug )
            {
                plugin_log_add(log,"encoding=%s description=%s\n",
                    encoding,description);
            }
            mvd_set_encoding( *mvd, encoding );
            if ( description != NULL )
            {
                int slen = strlen(description);
                int dlen = measure_from_encoding( description, 
                    slen, encoding );
                UChar *u_description = malloc ( dlen+sizeof(UChar) );
                if ( u_description != NULL )
                {
                    int nchars = convert_from_encoding( description, slen, 
                        u_description, (dlen/sizeof(UChar))+1, encoding );
                    if ( nchars > 0 )
                        mvd_set_description( *mvd, u_description );
                    if ( debug )
                        plugin_log_add(log,
                            "converted description %d chars long\n",nchars);
                    free( u_description );
                }
                else
                {
                    plugin_log_add(log,
                        "mvd_create: failed to encode description\n");
                    res = 0;
                }
            }
            // else leave description as default
            plugin_log_add(log,"mvd_create: success!\n");
        }
        else
        {
            plugin_log_add(log,"mvd_create: failed to create options map\n");
            res = 0;
        }
        hashmap_dispose( map, NULL );
    }
    else
    {
        plugin_log_add(log,"mvd_create: failed to create mvd\n");
        res = 0;
    }
    plugin_log_dispose( log );
    return res;
}
/**
 * Return a help message explaining what the parameters are
 */
char *help()
{
    return "options:\n\tencoding=<enc>\n\tdescription="
        "<string encoded in enc>\n\tdebug=1|0\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    return "version 0.1 (c) 2013 Desmond Schmidt\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *description()
{
    return "creates an empty mvd\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "create";
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test(int *p, int *f)
{
    MVD *mvd;
    unsigned char *scratch = malloc( SCRATCH_LEN );
    plugin_log *pl = plugin_log_create( scratch );
    int res = process( &mvd, "encoding=utf-16 description=test", 
        scratch, NULL, 0 );
    if ( res )
    {
        *p += 1;
        char *enc = mvd_get_encoding(mvd);
        if ( strcmp(enc,"utf-16")==0 )
            *p += 1;
        else
        {
            plugin_log_add(pl,"encoding should be utf-16 but it is %s\n",enc);
            *f += 1;
        }
    }
    else
    {
        plugin_log_add(pl,"mvd not created\n");
        *f += 1;
    }
    printf("%s",scratch );
    free( scratch );
}
#ifdef MVD_CREATE_TEST
int main( int argc, char **argv )
{
    MVD *mvd;
    unsigned char *output;
    int res = process( &mvd, "description=\"test\" encoding=\"utf-8\"", 
        &output, NULL, 0 );
    if ( res )
    {
        mvd_dispose( mvd );
        fprintf(stderr,"mvd successfully created!\n");
    }
    else
        fprintf(stderr,"failed to create MVD!\n");
    fprintf( stderr,"%s",output );
}
#endif
