/* 
 * File:   plugin_list.h
 * Author: desmond
 *
 * Created on January 15, 2013, 8:40 AM
 */

#ifndef PLUGIN_LIST_H
#define	PLUGIN_LIST_H

#ifdef	__cplusplus
extern "C" {
#endif
    
typedef struct plugin_list_struct plugin_list;

plugin_list *plugin_list_create();
void plugin_list_add( plugin_list *list, void *handle );
void plugin_list_dispose( plugin_list *list );
plugin *plugin_list_get( plugin_list *list, char *name );
void plugin_list_all( plugin_list *list );


#ifdef	__cplusplus
}
#endif

#endif	/* PLUGIN_LIST_H */

