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

