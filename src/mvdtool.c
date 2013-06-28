#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include "mvdtool.h"
#include "chunk_state.h"
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "pair.h"
#include "version.h"
#include "dyn_array.h"
#include "mvd.h"
#include "mvdfile.h"
#include "plugin.h"
#include "plugin_list.h"
#include "operation.h"
#include "plugin_log.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
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
/** list of available plugins */
plugin_list *plugins=NULL;
/** write in old MVD format */
int old = 0;
/** operations */
operation op=EMPTY;
/** optional user data */
unsigned char *user_data;
/** optional user data length */
int user_data_len;
static int file_size( const char *file_name )
{
    FILE *fp = fopen( file_name, "r" );
    long sz = -1;
    if ( fp != NULL )
    {
        fseek(fp, 0L, SEEK_END);
        sz = ftell(fp);
        fclose( fp );
    }
    return (int) sz;
}
/**
 * Read a file and return its contents as an allocated buffer
 * @param file the file's path
 * @param len VAR param to set to the length of the contents
 * @return the file's contents, caller to free
 */
char *read_file( char *file, int *len )
{
	FILE *fp = fopen(file,"r");
	char *buf = NULL;
	int res = 0;
	if ( fp == NULL )
	{
		fprintf(stderr, "couldn't open %s\n", file);
        return NULL;
	}
	*len = file_size(file);
	if ( *len > 0 )
	{
		buf = malloc( (*len)+1 );
		if ( buf != NULL )
			res = fread( buf, 1, *len, fp );
		if ( res != *len )
		{
			free( buf );
			buf = NULL;
		}
	}
	else
	{
		fprintf(stderr,"file %s length is 0\n", file);
	}
	return buf;
}
/**
 * Tell the user about how to use this program
 */
static void usage()
{
   fprintf(stdout,
    "usage: nmerge [-l] [-p] [-v <VERSION>] [-h COMMAND] [-u USER_DATA_FILE]\n"
       "[-m <MVD>] [-o <OPT-string>] -c <COMMAND>\n");
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
                    case 'u':
                        user_data = read_file( argv[++i], &user_data_len );
                        if ( user_data == NULL )
                            sane = 0;
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
                if ( mvdFile == NULL||command==NULL )
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
 * Open a single plugin
 * @param path the path to the plugin
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
    // without this it won't find any plugins
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
            
            printf("%s%s",plugin_description(plug),plugin_help(plug) );
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
 * Check whether a file exists
 * @param file the file to test
 * @return 1 if it does, 0 otherwise
 */
static int file_exists( const char *file )
{
    FILE *EXISTS = fopen( file, "r" );
    if ( EXISTS )
    {
        fclose( EXISTS );
        return 1;
    }
    return 0;
}
/**
 * Execute the preset command with its options
 */
void do_command()
{
    if ( plugins != NULL )
    {
        plugin *plug = plugin_list_get( plugins, command );
        if ( plug != NULL )
        {
            MVD *mvd = NULL;
            if ( file_exists(mvdFile) )
            {
                mvd = mvdfile_load( mvdFile );
                if ( mvd == NULL )
                    remove( mvdFile );
            }
            unsigned char *output = malloc( SCRATCH_LEN );
            if ( output != NULL )
            {
                int res = plugin_process( plug, &mvd, options, output, 
                    user_data, user_data_len );
                fprintf(stderr, "%s", output );
                if ( mvd != NULL && plugin_changes(plug) )
                {
                    res = mvdfile_save( mvd, mvdFile, 0 );
                    mvd_dispose( mvd );
                }
                free( output );
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
 * Test an mvd by 
 * 1. loading it 
 * 2. saving it in the old format 
 * 3. saving it in the new format
 * @param path the path of the src mvd
 * @param passed VAR param update with number of passed tests
 * @param failed VAR param update with the number of failed tests
 * @return 1 if the test succeeded
 */
static int test_mvd( char *path, int *passed, int *failed )
{
    MVD *mvd1 = mvdfile_load( path );
    if ( mvd1 != NULL )
    {
        *passed += 1;
        if ( mvd_test_versions(mvd1) )
            *passed += 1;
        else
            *failed += 1;
        char *write_path = strdup( path );
        if ( write_path != NULL )
        {
            char *dot_pos = strchr(write_path,'.');
            if ( dot_pos != NULL )
            {
                strcpy( dot_pos, ".old" );
                //printf("mvd_datasize=%d\n",mvd_datasize(mvd1,0));
                int res = mvdfile_save( mvd1, write_path, 1 );
                if ( res )
                {
                    *passed += 1;
                    strcpy( dot_pos, ".new" );
                    //printf("mvd_datasize=%d\n",mvd_datasize(mvd1,0));
                    res = mvdfile_save( mvd1, write_path, 0 );
                    if ( res )
                    {
                        *passed += 1;
                        MVD *mvd2 = mvdfile_load( write_path );
                        if ( mvd2 != NULL && mvd_equals(mvd1,mvd2) )
                        {
                            *passed += 1;
                            mvd_dispose( mvd2 );
                        }
                        else
                        {
                            printf("mvds %s and %s were NOT equal\n",
                                path,write_path);
                            *failed += 1;
                        }
                    }
                    else
                    {
                        *failed += 2;
                        fprintf(stderr,"mvdtool: failed to save new format\n");
                    }
                }
                else
                {
                    fprintf(stderr,"mvdtool: failed to save old format\n");
                    *failed += 2;
                }
            }
            free( write_path );
        }
        mvd_dispose( mvd1 );
    }
    else    // failed all tests in effect
        *failed += 4;
}
static void test_mvd_dir( char *folder, int *passed, int *failed )
{
    int res = 1;
    struct dirent *dp;
	DIR *dir = opendir(folder);
    if ( dir != NULL )
    {
        while ((dp=readdir(dir)) != NULL)
        {
            // is it a dynamic library?
            char *dot_pos = strrchr(dp->d_name,'.');
            if ( strcmp(dot_pos,".mvd")==0 )
            {
                int plen = strlen(folder)+2+strlen(dp->d_name);
                char *path = malloc(plen);
                if ( path != NULL )
                {
                    snprintf( path, plen, "%s/%s", folder, dp->d_name );
                    res = test_mvd( path, passed, failed );
                    free( path );
                    if ( !res )
                        break;
                }
            }
        }
        closedir(dir);
    }
    else
    {
        fprintf(stderr,"mvdtool: failed to open folder %s\n",folder);
    }
}
/**
 * Test the mvdtool class
 * @param passed VAR param number of passed tests
 * @param failed VAR param number of failed tests
 */
void test_mvdtool( int *passed, int *failed )
{
    // nmerge [-l] [-p] [-v COMMAND] [-h COMMAND] -m <MVD> -c <COMMAND> \n"
    //   "-o <OPT-string>
    static char *args1[] = {"nmergec","-l"};
    static char *args2[] = {"nmergec","-c","add","-m","kinglear.mvd","-o",
        "version=1"};
    static char *args3[] = {"nmergec","banana","-o","rubbish"};
    int sane = read_args(sizeof(args1)/sizeof(char*),args1);
    if ( !sane || op != LIST )
    {
        fprintf(stderr,"mvdtool: failed to read listing command\n");
        *failed += 1;
    }
    else
        *passed += 1;
    sane = read_args(sizeof(args2)/sizeof(char*),args2);
    if ( !sane || op != RUN || strcmp(mvdFile,"kinglear.mvd")!=0
        || strcmp(options,"version=1")!=0||strcmp(command,"add")!=0 )
    {
        fprintf(stderr,"mvdtool: failed to read add command and options\n");
        *failed += 1;
    }
    else
    {
        do_open_plugins();
        plugin *p = plugin_list_get( plugins, command );
        if ( p == NULL )
        {
            fprintf(stderr,"mvdtool: plugin %s not found\n",command);
            *failed += 1 ;
        }
        else
            *passed += 1;
        plugin_list_dispose( plugins );
    }
    if ( read_args(sizeof(args3)/sizeof(char*),args3) )
    {
        fprintf(stderr,"mvdtool: failed to detect bogus command\n");
        *failed += 1;
    }
    else
    {
        *passed += 1;
    }
    // test load and save of mvds
    test_mvd_dir( "mvds", passed, failed );
}
#endif