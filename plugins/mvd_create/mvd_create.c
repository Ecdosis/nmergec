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

static UChar ENCODING_KEY[] = {'e','n','c','o','d','i','n','g',0};
static UChar DESCRIPTION_KEY[] = {'d','e','s','c','r','i','p',
    't','i','o','n',0};

/**
 * Parse the options string into a map of key-value pairs
 * @param options the raw options string name=value
 * @return a map of key value pairs
 */
static hashmap *parse_options( char *options )
{
    hashmap *map = hashmap_create( 6, 0 );
    if ( map != NULL )
    {
        char *str = strdup( options );
        if ( str != NULL )
        {
            char *token = strtok( str, " " );
            while ( token != NULL )
            {
                char *value = strchr( token, '=' );
                if ( value != NULL )
                {
                    *value = '\0';
                    value++;
                    strip_quotes( value );
                    char *key = token;
                    int klen = strlen(key);
                    lowercase( key );
                    UChar *u_key = calloc( klen+1, sizeof(UChar) );
                    if ( u_key != NULL )
                    {
                        ascii_to_uchar( key, u_key, klen+1 );
                        hashmap_put( map, u_key, value );
                        free( u_key );
                    }
                }
                token = strtok( NULL, " " );
            }
        }
    }
    return map;
}
/**
 * Do the work of this plugin
 * @param mvd VAR param the mvd to create is stored here
 * @param options a string contianing the plugin's options
 * @param output VAR param set to NULL if not needed else the output
 * @param data ignored
 * @param data_len ignored
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char **output, 
unsigned char *data, size_t data_len )
{
    int res = 1;
    plugin_log *log = plugin_log_create();
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
            mvd_set_encoding( *mvd, encoding );
            if ( description != NULL )
            {
                int dlen = measure_from_encoding( description, 
                    strlen(description), encoding );
                UChar *u_description = malloc ( dlen+sizeof(UChar) );
                if ( u_description != NULL )
                {
                    int nchars = convert_from_encoding( description, dlen, 
                        u_description, dlen+1, encoding );
                    if ( nchars > 0 )
                        mvd_set_description( *mvd, u_description );
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
    }
    else
    {
        plugin_log_add(log,"mvd_create: failed to create mvd\n");
        res = 0;
    }
    *output = plugin_log_buffer(log);
    plugin_log_dispose( log );
    return res;
}
/**
 * Print a help message to stdout explaining what the paramerts are
 */
void help()
{
    printf("help\n");
}
/**
 * Report the plugin's version and author to stdout
 */
void plug_version( char **output )
{
    plugin_log *log = plugin_log_create();
    plugin_log_add( "version 0.1 (c) 2013 Desmond Schmidt\n");
    *output = plugin_log_buffer();
    plugin_log_dispose( log );
}
/**
 * Report the plugin's version and author to stdout
 * @param output VAR param: set to description string
 */
void plug_description( char **output )
{
    plugin_log *log = plugin_log_create();
    plugin_log_add( "creates an empty mvd\n");
    *output = plugin_log_buffer();
    plugin_log_dispose( log );
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
int test(int *p,int *f)
{
    return 1;
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
