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

void plugin_log( char *fmt, ... );
void plugin_log_clear();
char *plugin_log_buffer();



#ifdef	__cplusplus
}
#endif

#endif	/* PLUGIN_LOG_H */

