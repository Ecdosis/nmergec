
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_log.h"
#define SCRATCH_LEN 1024
struct plugin_log_struct
{
    char *scratch;
    // message buffer len
    int pos;
};
/**
 * Create a plugin log object
 * @return the log
 */
plugin_log *plugin_log_create()
{
    plugin_log *log = calloc( sizeof(plugin_log), 1 );
    if ( log == NULL )
        fprintf(stderr,"plugin_log: failed to create log\n");
    else
    {
        log->scratch = malloc( SCRATCH_LEN );
        if ( log->scratch != NULL )
            log->scratch[0] = 0;
        else
        {
            fprintf(stderr,"plugin_log: failed to allocate\n");
            free( log );
            log = NULL;
        }
    }
    return log;
}
/**
 * Dispose of the plugin log
 * @param log the log in question
 */
void plugin_log_dispose( plugin_log *log )
{
    if ( log != NULL )
        free( log );
    // leave the buffer: caller must dispose
}
/**
 * Add to the plugin log
 * @param log the log to add to
 * @param fmt the format
 * @param ... the arguments to insert via the format
 */
void plugin_log_add( plugin_log *log, char *fmt, ... )
{
    char str[128];
    va_list ap;
    va_start(ap, fmt); 
    int nconvs=0;
    int i,fmt_len = strlen(fmt);
    for ( i=0;i<fmt_len;i++ )
        if ( fmt[i]=='%' )
            nconvs++;
    if ( nconvs > 0 )
    {
        vsnprintf( str, 128, fmt, ap );
        if ( strlen(str)+log->pos < SCRATCH_LEN )
        {
            strncat( log->scratch, str, SCRATCH_LEN-log->pos );
            log->pos = strlen( log->scratch );
        }
    }
    else
    {
        if ( strlen(fmt)+log->pos < SCRATCH_LEN )
        {
            strncat( log->scratch, fmt, SCRATCH_LEN-log->pos );
            log->pos = strlen( log->scratch );
        }
    }
    va_end( ap );
}
void plugin_log_clear( plugin_log *log )
{
    log->pos = 0;
    log->scratch[0] = 0;
}
char *plugin_log_buffer( plugin_log *log )
{
    return log->scratch;
}