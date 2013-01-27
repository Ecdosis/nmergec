#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "bitset.h"
#include "link_node.h"
#include "mvd/pair.h"
#include "mvd/version.h"
#include "mvd/mvd.h"
#include "plugin.h"
#ifdef MVD_TEST
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
        plug->version = dlsym(handle, "version");
        if ( plug->version == NULL )
            fprintf(stderr,"plugin: failed to find version function: %s\n", 
                dlerror() );
		plug->test = dlsym(handle, "test");
        if ( plug->test == NULL )
            fprintf(stderr,"plugin: failed to find test function: %s\n", 
                dlerror() );
		plug->name = dlsym(handle, "name");
        if ( plug->name == NULL )
            fprintf(stderr,"plugin: failed to find name function: %s\n", 
                dlerror() );
    }
    return plug;
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
 * @param mvd the mvd to read or modify
 * @param options a string containing the plugin options
 * @param output VAR param containing the output in a byte array
 * @return 1 if it worked else 0
 */
int plugin_process( plugin *plug, MVD *mvd, char *options, 
    unsigned char **output )
{
	return (plug->process)(mvd,options,output);
}
/**
 * Print the help for this plugin
 * @param plug the plugin in question
 */
void plugin_help( plugin *plug )
{
	(plug->help)();
}
/**
 * Print plugin version and authorship information
 * @param plug the plugin in question
 */
void plugin_version( plugin *plug )
{
	return (plug->version)();
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

