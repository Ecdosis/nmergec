/* 
 * File:   plugin_log.h
 * Author: desmond
 *
 * Created on February 21, 2013, 1:32 PM
 */

#ifndef PLUGIN_LOG_H
#define	PLUGIN_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif
#define SCRATCH_LEN 1024

typedef struct plugin_log_struct plugin_log;
plugin_log *plugin_log_create();
void plugin_log_dispose( plugin_log *log );
void plugin_log_add( plugin_log *log, char *fmt, ... );
void plugin_log_clear( plugin_log *log );
char *plugin_log_buffer( plugin_log *log );
int plugin_log_pos( plugin_log *log );


#ifdef	__cplusplus
}
#endif

#endif	/* PLUGIN_LOG_H */

