#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "mvd/pair.h"
#include "mvd/version.h"
#include "mvd/mvd.h"
#include "plugin.h"
#include "encoding.h"
#include "suffixtree.h"
#include "plugin_log.h"

// the version ID - a /-delimited string
static char *vid=NULL;
// description of the version to be added
static UChar *description=NULL;
// the text itself - we use wide UTF-16 as the basic char type
static UChar *text;
// number of wchar_t characters in text 
static size_t tlen;
// encoding of the input file
static char *encoding="utf-8";

/**
 * Convert a writable string to lowercase
 * @param str the string to lowercase
 */
static void lowercase( char *str )
{
    int i,len = strlen(str);
    for ( i=0;i<len;i++ )
        str[i] = tolower(str[i] );
}
/**
 * Get the length of an open file
 * @param fp an open FILE handle
 * @return file length if successful, else 0
 */
static int file_length( FILE *fp )
{
	int length = 0;
    int res = fseek( fp, 0, SEEK_END );
	if ( res == 0 )
	{
		long long_len = ftell( fp );
		if ( long_len > INT_MAX )
        {
			fprintf( stderr,"mvdfile: file too long: %ld", long_len );
            length = res = 0;
        }
		else
        {
            length = (int) long_len;
            if ( length != -1 )
                res = fseek( fp, 0, SEEK_SET );
            else
                res = 1;
        }
	}
	if ( res != 0 )
    {
		fprintf(stderr, "add: failed to read file. error %s\n",
            strerror(errno) );
        length = 0;
    }
	return length;
}
/**
 * Load the given text file 
 * @param path the path to the file
 * @param encoding the encoding of the file to be added
 * @return 1 if it loaded OK, else 0
 */
static int load_text( char *path, char *encoding )
{
    int res = 1;
    FILE *fp = fopen( path, "r" );
    if ( fp != NULL )
    {
        int len = file_length( fp );
        if ( len > 0 )
        {
            char *data = malloc( len );
            if ( data != NULL )
            {
                int nitems = fread( data, 1, len, fp );
                if ( nitems == len )
                {
                    int nchars=0;
                    int nbytes = measure_from_encoding( data, len, encoding );
                    UChar *text = malloc( nbytes+sizeof(UChar) );
                    if ( text != NULL )
                    {
                        nchars = convert_from_encoding(data,len,text, 
                            nbytes+sizeof(UChar),encoding)/sizeof(UChar);        
                        if ( nchars==0 )
                        res = 0;
                    }
                    else
                    {
                        plugin_log("mvd_add: failed to allocate unicode buffer\n");
                        res = 0;
                    }
                }
            }
            else
            {
                res = 0;
                plugin_log("add: failed to allocate %d bytes\n",len);
            }
        }
        fclose( fp );
    }
    else
    {
        plugin_log("add: couldn't open %s\n",path);
        res = 0;
    }
    return res;
}
/**
 * Parse the options
 * @param option_string the options as passed-in, separated by "=" and spaces
 * @return 1 if the options were sane
 */
static int parse_options( char *option_string )
{
    int sane = 1;
    char *opt = strtok( option_string, " ");
    while ( opt != NULL && sane )
    {
        char *equals = strchr( opt, '=' );
        if ( equals != NULL )
        {
            // read option name
            int name_len = (equals-opt);
            char *name = malloc( name_len+1 );
            if ( name != NULL )
            {
                strncpy( name, option_string, name_len );
                name[name_len] = 0;
            }
            else
                sane = 0;
            // read option value
            int value_len = (int)(&option_string[strlen(option_string)]-&equals[1]);
            char *value = malloc( value_len+1 );
            if ( value != NULL )
            {
                strncpy( value, &equals[1], value_len );
                value[value_len] = 0;
            }
            else
                sane = 0;
            if ( name != NULL && value != NULL )
            {
                lowercase( name );
                if ( strcmp(name,"text")==0 )
                    sane = load_text(value,encoding);
                else if ( strcmp(name,"vid") )
                {
                    vid = strdup(value);
                    sane = (version_id != NULL && strlen(vid)>0);
                }
                else if ( strcmp(name,"description")==0 )
                {
                    int nchars=0;
                    int slen = strlen(value);
                    int nbytes = measure_from_encoding( value, slen, "utf-8" );
                    description = malloc( nbytes+sizeof(UChar) );
                    if ( description != NULL )
                    {
                        nchars = convert_from_encoding(value,slen,description, 
                            nbytes+sizeof(UChar),"utf-8")/sizeof(UChar);        
                        if ( nchars==0 )
                            plugin_log("mvd_add: failed to convert "
                                "description string\n");
                    }
                    else
                        plugin_log("mvd_add: failed to allocate unicode buffer\n");
                    sane = (description!=NULL);
                }
                else if ( strcmp(name,"encoding")==0 )
                {
                    encoding = strdup(value);
                    sane = (encoding!=NULL);
                }
                else
                {
                    plugin_log("mvd_add: unknown option %s\n",name);
                    sane = 0;
                }
            }
            if ( name != NULL )
                free( name );
            if ( value != NULL )
                free( value );
        }
        else
            sane = 0;
        opt = strtok( NULL, " " );        
    }
    return sane;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulate or read
 * @param options a string containing the plugin's options
 * @param output VAR param set to NULL if not needed else the output
 * @param data data passed directly in or NULL
 * @param data_len length of data
 * @return 1 if the process completed successfully
 */
int process( MVD *mvd, char *options, unsigned char **output, 
    unsigned char *data, size_t data_len )
{
    plugin_log_clear();
    int res = parse_options( options );
    if ( res && data != NULL && data_len > 0 )
    {
        size_t *nchars;
        char *mvd_encoding = mvd_get_encoding(mvd);
        if ( strcmp(mvd_encoding,encoding)!=0 )
        {
            plugin_log("file's encoding %s does not match mvd's (%s):"
                " assimilating...\n",encoding,mvd_encoding);
        }
        int nbytes = measure_from_encoding( (char*)data, data_len, encoding );
        text = malloc( nbytes+sizeof(UChar) );
        if ( text != NULL )
        {
            nbytes = convert_from_encoding(data, data_len, text, 
                nbytes/sizeof(UChar)+1, encoding)/sizeof(UChar);
            if ( nbytes > 0 )
            {
                tlen = nbytes/sizeof(UChar);
                suffixtree *st = suffixtree_create( text, tlen );
                // then process the suffix tree...
            }
            else
                res = 0;
        }
        else
        {
            plugin_log("failed to allocate uchar buffer\n");
            res = 0;
        }
    }
    *output = plugin_log_buffer();
    return res;
}
/**
 * Print a help message to stdout explaining what the parameters are
 */
char *help()
{
    return "options:\n"
        "text=<file>: file to add to existing MVD (optional)\n"
        "vid=<vid>: version ID as a /-delimited path\n"
        "description=<utf8-string>: description of new version\n"
        "encoding=<canonical-encoding-name>: the text encoding\n";
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
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test( int *p,int *f )
{
    return 1;
}
