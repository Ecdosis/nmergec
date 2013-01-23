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
#include "mvd/pair.h"
#include "mvd/mvd.h"
#include "plugin.h"
#include "plugin_list.h"
#ifdef MVD_TEST
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
        list->plugins = malloc( sizeof(plugin*)*(list->block_size) );
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
 */
void plugin_list_add( plugin_list *list, void *handle )
{
    int i;
    if ( list->num_plugins+1 > list->block_size )
    {
        int new_size = list->block_size+8;
        plugin **temp = malloc(sizeof(plugin*)*new_size );
        if ( temp == NULL )
        {
            fprintf(stderr,
                "plugin_list: failed to reallote list of plugins\n");
        }
        list->block_size = new_size;
        for ( i=0;i<list->num_plugins;i++ )
            temp[i] = list->plugins[i];
        free( list->plugins );
        list->plugins = temp;
    }
	list->plugins[list->num_plugins++] = plugin_create( handle );
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
        printf("%s\n",plug_name);
	}
}
