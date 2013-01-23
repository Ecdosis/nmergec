#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include "mvdtool.h"
#include "mvd/chunk_state.h"
#include "bitset.h"
#include "mvd/pair.h"
#include "mvd/mvd.h"
#include "mvd/mvdfile.h"
#include "plugin.h"
#include "plugin_list.h"
#include "operation.h"

#ifdef MVD_DEBUG
#include "memwatch.h"
#endif
#define PATH_LEN 128

#ifndef PLUGIN_DIR
#define PLUGIN_DIR "/usr/local/lib/nmerge-plugins"
#endif

#ifdef __ELF__
#define LIB_SUFFIX ".so"
#else
#define LIB_SUFFIX ".dylib"
#endif

/** name of plugin command */
static char *command=NULL;
/** options for command */
static char *options=NULL;
/** name of mvd file */
static char *mvdFile=NULL;
/** list of available modules */
plugin_list *plugins=NULL;
/** write in old MVD format */
int old = 0;
/** operations */
operation op=EMPTY;
/**
 * Tell the user about how to use this program
 */
static void usage()
{
   fprintf(stdout,
    "usage: nmerge [-l] [-p] [-v COMMAND] [-h COMMAND] -m <MVD> -c <COMMAND> \n"
       "-o <OPT-string> \n");
}
/**
 * Read in the arguments
 * @param argc number of arguments plus 1
 * @param agv the argument array
 * @return 1 if the arguments were valid
 */
static int read_args( int argc, char **argv ) 
{
    int sane = 1;
    if ( argc > 1 )
    {
        int i=1;
        while ( i<argc )
        {
            if ( argv[i][0] =='-' && strlen(argv[i])>1 )
            {
                switch ( argv[i][1] )
                {
                    case 'c':
                        command = argv[++i];
                        op = RUN;
                        break;
                    case 'o':
                        options = argv[++i];
                        break;
                    case 'p':
                        old = 1;
                        break;
                    case 'm':
                        mvdFile = argv[++i];
                        break;
                    case 'l':
                        op = LIST;
                        break;
                    case 'h':
                        op = HELP;
                        command = argv[++i];
                        break;
                    case 'v':
                        op = VERSION;
                        command = argv[++i];
                        break;
                    default:
                        fprintf(stderr, "mvdtool: unknown option %c\n",
                            argv[i][1] );
                        break;
                }
            }
            else 
            {
                fprintf(stderr,"mvdtool: unexpected commandline token %s\n",
                    argv[i]);
                sane = 0;
            }
            if ( !sane )
                break;
            i++;
        }
        switch ( op )
        {
            case RUN:
                if ( mvdFile == NULL||command==NULL||options==NULL )
                    sane = 0;
                break;
            case HELP:
                if ( command == NULL )
                    sane = 0;
                break;
            case VERSION:
                if ( command == NULL )
                    sane = 0;
                break;
        }
    }
    else
        sane = 0;
    return sane;
}
/**
 * Open a single bot-module
 * @param path the path to the module
 * @return the result of loading it: 1 if success, 0 otherwise
 */
static void open_plugin( char *path )
{
	void *handle = dlopen( path, RTLD_LOCAL|RTLD_LAZY );
	if ( handle != NULL )
	{
		if ( plugins == NULL )
			plugins = plugin_list_create();
		plugin_list_add( plugins, handle );
	}
	else
		fprintf(stderr, 
            "mvdtool: failed to read plugin path %s. error: %s\n",
			path,dlerror() );
}
/**
 * Look for plugins to open. PLUGIN_DIR is supplied
 * during compilation via -D
 */
static void do_open_plugins()
{
	struct dirent *dp;
	DIR *dir = opendir(PLUGIN_DIR);
    int suffix_len = strlen(LIB_SUFFIX);
    // without this it won't find any modules
	setenv("LD_LIBRARY_PATH",PLUGIN_DIR,1);
	while ((dp=readdir(dir)) != NULL)
	{
		char path[PATH_LEN];
		char suffix[32];
		int len;
		snprintf( path, PATH_LEN, "%s/%s", PLUGIN_DIR, dp->d_name );
		len = strlen( dp->d_name );
		if ( len > suffix_len )
		{
            // is it a dynamic library?
            char *dot_pos = strrchr(dp->d_name,'.');
			strncpy( suffix, dot_pos, 32 );
			if ( strcmp(suffix,LIB_SUFFIX)==0 )
				open_plugin( path );
		}
	}
	closedir(dir);
}
/**
 * Print the specified plugin's help message
 */
void do_help()
{
    if ( plugins != NULL )
    {
        plugin *plug = plugin_list_get( plugins, command );
        if ( plug != NULL )
            plugin_help( plug );
    }
}/**
 * Display the specified plugin version
 */
void do_version()
{
    if ( plugins != NULL )
    {
        plugin *plug = plugin_list_get( plugins, command );
        if ( plug != NULL )
            plugin_version( plug );
    }
}
/**
 * Execute the preset command with its options
 */
void do_command()
{
    if ( plugins != NULL )
    {
        unsigned char *output;
        plugin *plug = plugin_list_get( plugins, command );
        if ( plug != NULL )
        {
            MVD *mvd = mvdfile_load( mvdFile );
            if ( mvd != NULL )
            {
                int res = plugin_process( plug, mvd, options, &output );
                if ( res && output != NULL )
                    printf( "%s\n", output );
            }
        } 
    }
}
/**
 * List available plugins
 */
static void list_plugins()
{
    if ( plugins != NULL )
        plugin_list_all( plugins );
    else
        fprintf(stderr,"mvdtool: no plugins found\n");
}

#ifndef MVD_TEST
int main( int argc, char **argv )
{
    if ( read_args(argc,argv) )
    {
        do_open_plugins();
        switch ( op )
        {
            case EMPTY:
                usage();
                break;
            case RUN:
                do_command();
                break;
            case VERSION:
                do_version();
                break;
            case LIST:
                list_plugins();
                break;
            case HELP:
                do_help();
                break;
        }
        plugin_list_dispose( plugins );
    }
    else
        usage();
}
#else
/**
 * Test the mvdtool class
 * @param passed VAR param number of passed tests
 * @param failed VAR param number of failed tests
 * @return 1 if all was OK
 */
int test_mvdtool( int *passed, int *failed )
{
    return 1;
}
#endif