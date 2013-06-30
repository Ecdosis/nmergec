/*
 *  NMergeC is Copyright 2013 Desmond Schmidt
 * 
 *  This file is part of NMergeC. NMergeC is a C commandline tool and 
 *  static library and a collection of dynamic libraries for merging 
 *  multiple versions into multi-version documents (MVDs), and for 
 *  reading, searching and comparing them.
 *
 *  NMergeC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NMergeC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_log.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
struct plugin_log_struct
{
    // we don't own this
    char *scratch;
    // message buffer len
    int pos;
};
/**
 * Create a plugin log object
 * @param buffer a buffer of length SCRATCH_LEN - we don't own this
 * @return the log
 */
plugin_log *plugin_log_create( char *buffer )
{
    plugin_log *log = calloc( sizeof(plugin_log), 1 );
    if ( log == NULL )
        fprintf(stderr,"plugin_log: failed to create log\n");
    else
        log->scratch = buffer;
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
int plugin_log_pos( plugin_log *log )
{
    return log->pos;
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
            memcpy( &log->scratch[log->pos], str, strlen(str) );
            log->pos += strlen(str);
            log->scratch[log->pos] = 0;
        }
    }
    else
    {
        if ( strlen(fmt)+log->pos < SCRATCH_LEN )
        {
            memcpy( &log->scratch[log->pos], fmt, strlen(fmt) );
            log->pos += strlen( fmt );
            log->scratch[log->pos] = 0;
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