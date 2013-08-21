#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "pair.h"
#include "version.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#ifdef MVD_TEST
#include <string.h>
#include <dirent.h>
#endif
#ifdef MEMWATCH
#include "memwatch.h"
#endif


struct plugin_struct
{
	void *handle;
    plugin_process_type process;
	plugin_help_type help;
	plugin_version_type version;
    plugin_test_type test;
    plugin_name_type name;
    plugin_description_type description;
    plugin_changes_type changes;
};
/**
 * Create a plugin from a loaded library handle
 * @param handle handle return from dlopen
 * @return a module object or NULL on failure
 */
plugin *plugin_create( void *handle )
{
	plugin *plug = malloc( sizeof(plugin) );
	if ( plug == NULL )
		fprintf(stderr,"plugin: failed to allocate object\n");
	else
    {
        plug->handle = handle;
        plug->process = dlsym(handle, "process");
        if ( plug->process == NULL )
            fprintf(stderr,"plugin: failed to find process function: %s\n", 
                dlerror() );
        plug->help = dlsym(handle, "help");
        if ( plug->help == NULL )
            fprintf(stderr,"plugin: failed to find help function: %s\n", 
                dlerror() );
        plug->version = dlsym(handle, "plug_version");
        if ( plug->version == NULL )
            fprintf(stderr,"plugin: failed to find version function: %s\n", 
                dlerror() );
		plug->description = dlsym(handle, "description");
        if ( plug->description == NULL )
            fprintf(stderr,"plugin: failed to find description function: %s\n", 
                dlerror() );
		plug->test = dlsym(handle, "test");
        if ( plug->test == NULL )
            fprintf(stderr,"plugin: failed to find test function: %s\n", 
                dlerror() );
		plug->name = dlsym(handle, "name");
        if ( plug->name == NULL )
            fprintf(stderr,"plugin: failed to find name function: %s\n", 
                dlerror() );
        plug->changes = dlsym(handle, "changes");
        if ( plug->changes == NULL )
            fprintf(stderr,"plugin: failed to find changes function: %s\n", 
                dlerror() );
    }
    return plug;
}
/**
 * Check that all our entry points are set
 * @param plug the plugin to check
 * @param entry set this to the offending entry point
 * @return 1 if all was OK
 */
int plugin_check_handles( plugin *plug, char **entry )
{
    int res = 1;
    if ( plug->process == NULL )
    {
        *entry = "process";
        res = 0;
    }
    if ( plug->help == NULL )
    {
        *entry = "help";
        res = 0;
    }
    if ( plug->version == NULL )
    {
        *entry = "version";
        res =0;
    }
	if ( plug->description == NULL )
    {
        *entry = "description";
        res = 0;
    }
    if ( plug->test == NULL )
    {
        *entry = "test";
        res = 0;
    }
    if ( plug->name == NULL )
    {
        *entry = "name";
        res = 0;
    }
    if ( plug->changes == NULL )
    {
        *entry = "changes";
        res = 0;
    }
    return res;
}
/**
 * Close a plugin and free up its descriptor
 * @param mod the plugin instance to close
 */
void plugin_dispose( plugin *plug )
{
	if ( plug->handle != NULL )
        dlclose( plug->handle );
	free( plug );
}
/**
 * Run the plugin
 * @param plug the plugin in question
 * @param mvd VAR param the mvd to read or modify
 * @param options a string containing the plugin options
 * @param output VAR param containing the output in a byte array
 * @param data optional user supplied data or NULL
 * @param data_len optional length of user data or 0
 * @return 1 if it worked else 0
 */
int plugin_process( plugin *plug, MVD **mvd, char *options, 
    unsigned char *output, unsigned char *data, size_t data_len )
{
    return (plug->process)(mvd,options,output,data,data_len);
}
/**
 * Print the help for this plugin
 * @param plug the plugin in question
 */
char *plugin_help( plugin *plug )
{
	return (plug->help)();
}
/**
 * Does this plugin change the MVD so that it needs saving?
 * @param plug the plugin in question
 * @return 1 if it changes the MVD else 0
 */
int plugin_changes( plugin *plug )
{
	return (plug->changes)();
}
/**
 * Print plugin version and authorship information
 * @param plug the plugin in question
 */
char *plugin_version( plugin *plug )
{
	return (plug->version)();
}
/**
 * Print plugin version and authorship information
 * @param plug the plugin in question
 */
char *plugin_description( plugin *plug )
{
	return (plug->description)();
}
/**
 * Test this plugin
 * @param plug the plugin in question
 * @param passed VAR param increment number of passed tests
 * @param failed VAR param increment number of failed tests
 * @return 1 if it worked else 0
 */
int plugin_test( plugin *plug, int *passed, int *failed )
{
    return (plug->test)( passed, failed );
}
/**
 * Get the name of this plugin
 * @return its name as a string
 */
char *plugin_name( plugin *plug )
{
    return (plug->name)();
}
#ifdef MVD_TEST
#ifdef __APPLE__
#define LIB_SUFFIX ".dylib"
#else
#define LIB_SUFFIX ".so"
#endif
/**
 * List available plugins
 */
int get_plugins( char *folder, char ***paths, int *num )
{
    int res = 1;
    struct dirent *dp;
	DIR *dir = opendir(folder);
    *num = 0;
    if ( dir != NULL )
    {
        while ((dp=readdir(dir)) != NULL)
        {
            // is it a dynamic library?
            char *dot_pos = strrchr(dp->d_name,'.');
            if ( strcmp(dot_pos,LIB_SUFFIX)==0 )
            {
                (*num)++;
            }
        }
        if ( *num > 0 )
        {
            rewinddir(dir);
            *paths = calloc( *num, sizeof(char*) );
            if ( paths != NULL )
            {
                int i = 0;
                while (res&&(dp=readdir(dir)) != NULL)
                {
                    // is it a dynamic library?
                    char *dot_pos = strrchr(dp->d_name,'.');
                    if ( strcmp(dot_pos,LIB_SUFFIX)==0 )
                    {       
                        int plen = strlen(folder)+2+strlen(dp->d_name);
                        (*paths)[i] = malloc(plen);
                        if ( (*paths)[i] != NULL )
                        {
                            snprintf( (*paths)[i++], plen, 
                                "%s/%s", folder, dp->d_name );
                        }
                        else
                            res = 0;
                    }
                }
            }
            else
                res = 0;
        }
        closedir(dir);
    }
    else
        res = 0;
    return res;
}
void test_plugin( int *passed, int *failed )
{
    char **paths;
    int num_paths;
    int res = get_plugins("/usr/local/lib/nmerge-plugins",&paths,&num_paths);
    if ( res && num_paths >0 )
    {
        int i;
        for ( i=0;i<num_paths;i++ )
        {
            char *entry;
            void *handle = dlopen( paths[i], RTLD_LOCAL|RTLD_LAZY );
            if ( handle != NULL )
            {
                plugin *p = plugin_create( handle );
                if ( p == NULL )
                {
                    fprintf(stderr,
                        "plugin: failed to load plugin %s\n",paths[i]);
                    (*failed)++;
                    res = 0;
                    break;
                }
                else if ( !plugin_check_handles(p,&entry) )
                {
                    fprintf(stderr,
                        "plugin: failed to find entry point %s\n",entry);
                    (*failed)++;
                    res = 0;
                    break;
                }
                else
                    plugin_dispose( p );
            }
            else
            {
                fprintf(stderr,"plugin: failed to load plugin %s\n",paths[i]);
                (*failed)++;
                res = 0;
                break;
            }
        }
        if ( res ) 
            (*passed)++;
        for ( i=0;i<num_paths;i++ )
            if ( paths[i] != NULL )
                free( paths[i] );
        free( paths );
    }
    else
    {
        fprintf(stderr,"plugin: failed to find any plugins\n");
        (*failed)++;
    }
}
#endif
