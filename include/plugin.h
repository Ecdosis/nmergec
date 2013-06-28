/* 
 * File:   plugin.h
 * Author: desmond
 *
 * Created on January 15, 2013, 7:03 AM
 */

#ifndef PLUGIN_H
#define	PLUGIN_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct plugin_struct plugin;
/* function typedefs used in plugin.c */
typedef int (*plugin_process_type)( MVD **mvd, char *options, 
        unsigned char *output, unsigned char *data, size_t data_len );
typedef char *(*plugin_help_type)();
typedef char *(*plugin_version_type)();
typedef char *(*plugin_name_type)();
typedef char *(*plugin_description_type)();
typedef int (*plugin_test_type)(int *p,int *f);
typedef int (*plugin_changes_type)();
plugin *plugin_create( void *handle );
void plugin_dispose( plugin *plug );
int plugin_process( plugin *plug, MVD **mvd, char *options, 
    unsigned char *output, unsigned char *data, size_t data_len );
char *plugin_help( plugin *plug );
char *plugin_version( plugin *plug );
char *plugin_description( plugin *plug );
int plugin_test( plugin *plug, int *passed, int *failed );
char *plugin_name( plugin *plug );
int plugin_changes( plugin *plug );

#ifdef	__cplusplus
}
#endif

#endif	/* PLUGIN_H */

