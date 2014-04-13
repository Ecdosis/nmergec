/**
 * @file
 * A plugin list is just a list of plugins.
 * Maintain the list as we dynamically discover new
 * ones during startup and also retrieve them etc.
 * @author Desmond Schmidt 15/1/13
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "plugin_list.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct plugin_list_struct
{
	plugin **plugins;
	int num_plugins;
    int block_size;
};
/**
 * Create a plugin list
 * @return a plugin list object or NULL on failure
 */
plugin_list *plugin_list_create()
{
	struct plugin_list_struct *list = calloc( 1, sizeof(plugin_list) );
	if ( list == NULL )
	{
		fprintf(stderr,"plugin_list: failed to allocate plugin list\n" );
	}
    else
    {
        list->block_size = 12;
        list->plugins = calloc( list->block_size,sizeof(plugin*) );
        if ( list->plugins == NULL )
        {
            fprintf(stderr,"plugin_list: failed to allocate plugin array\n" );
            free( list );
            list = NULL;
        }
    }
	return list;
}
/**
 * Add a new plugin to the list
 * @param list the list object to add it to
 * @param handle handle to a loaded dll
 * @return 1 if it worked else 0
 */
int plugin_list_add( plugin_list *list, void *handle )
{
    int res = 1;
    if ( list->num_plugins == list->block_size )
    {
        int new_size = list->block_size+8;
        plugin **temp = calloc( new_size,sizeof(plugin*) );
        if ( temp == NULL )
        {
            fprintf(stderr,
                "plugin_list: failed to reallocate list of plugins\n");
            res = 0;
        }
        else
        {
            int i;
            list->block_size = new_size;
            for ( i=0;i<list->num_plugins;i++ )
            {
                temp[i] = list->plugins[i];
            }
            free( list->plugins );
            list->plugins = temp;
        }
    }
	list->plugins[list->num_plugins++] = plugin_create( handle );
    return res;
}
/**
 * Close all plugin connections and dispose of ourselves
 * @param list the list in question
 */
void plugin_list_dispose( plugin_list *list )
{
	int i;
	for ( i=0;i<list->num_plugins;i++ )
	{
		plugin_dispose( list->plugins[i] );
	}
	free( list->plugins );
	free( list );
}
/**
 * Get an nmerge plugin given its name
 * @param list the list in question
 * @param name the name of the plugin or NULL
 */
plugin *plugin_list_get( plugin_list *list, char *name )
{
    int i;
	for ( i=0;i<list->num_plugins;i++ )
	{
		const char *plug_name = plugin_name(list->plugins[i]);
		if ( strcmp(plug_name,name)==0 )
			return list->plugins[i];
    }
	fprintf(stderr,"plugin_list: failed to find plugin %s\n",name);
	return NULL;
}
/**
 * List loaded plugins
 * @param list the list in question
 */
void plugin_list_all( plugin_list *list )
{
    int i;
    for ( i=0;i<list->num_plugins;i++ )
	{
		const char *plug_name = plugin_name(list->plugins[i]);
        printf("%s - %s",plug_name, plugin_description(list->plugins[i]));
	}
}
// NB: can't really test this without creating plugins per se
#ifdef MVD_TEST
void test_plugin_list( int *passed, int *failed )
{
    // 1. list plugins in /usr/local/lib/nmerge-plugins/
    char **paths;
    int num_paths;
    int res = get_plugins("/usr/local/lib/nmerge-plugins",&paths,&num_paths);
    if ( res && num_paths >0 )
    {
        int i;
        plugin_list *pl = plugin_list_create();
        if ( pl != NULL )
        {
            for ( i=0;i<num_paths;i++ )
            {
                void *handle = dlopen( paths[i], RTLD_LOCAL|RTLD_LAZY );
                if ( handle != NULL )
                    plugin_list_add( pl, handle );
                else
                    res = 0;
            }
            if ( pl->num_plugins != num_paths )
            {
                fprintf(stderr,
                    "plugin_list: found %d plugins but should be %d\n",
                    pl->num_plugins,num_paths);
                res = 0;
            }
            plugin_list_dispose( pl );
        }
    }
    if ( res )
        (*passed)++;
    else
        (*failed)++;
}
/*
int main( int argc, char **argv )
{
    int passed=0,failed=0;
    test_plugin_list( &passed, &failed );
    printf("passed=%d failed=%d\n",passed,failed);
}
*/
#endif