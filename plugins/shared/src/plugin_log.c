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
#define SCRATCH_LEN 1024

struct plugin_log_struct
{
    // buffer to write messages in
    char *scratch;
    // message buffer len
    int pos;
    int allocated;
};
/**
 * Create a plugin log object
 * @param buffer a buffer initially of length SCRATCH_LEN
 * @return the log
 */
plugin_log *plugin_log_create()
{
    plugin_log *log = calloc( sizeof(plugin_log), 1 );
    if ( log == NULL )
        fprintf(stderr,"plugin_log: failed to create log\n");
    else
    {
        log->scratch = calloc( 1, SCRATCH_LEN );
        if ( log->scratch == NULL )
        {
            fprintf(stderr,"plugin_log: failed to create buffer\n");
            plugin_log_dispose(log);
            log = NULL;
        }
        else
            log->allocated = SCRATCH_LEN;
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
    {
        if ( log->scratch != NULL )
            free( log->scratch );
        free( log );
    }
}
/**
 * Get the current write position (or length) of the log
 * @param log the log to query
 * @return the length of its text
 */
int plugin_log_pos( plugin_log *log )
{
    return log->pos;
}
/**
 * Resize the log
 * @param log the log in question
 * @param slen the number of bytes required
 * @return 1 if it worked else 0
 */
static int plugin_log_resize( plugin_log *log, int slen )
{
    int res = 0;
    int new_len = slen+SCRATCH_LEN+log->allocated;
    char *new_buf = calloc( new_len, 1 );
    if ( new_buf != NULL )
    {
        strncpy( new_buf, log->scratch, log->pos );
        free( log->scratch );
        log->scratch = new_buf;
        res = 1;
    }
    return res;
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
        int slen = strlen(str);
        if ( slen+log->pos < log->allocated || plugin_log_resize(log,slen) )  
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
#ifdef MVD_TEST
#include <math.h>
void plugin_log_test( int *passed, int *failed )
{
    const char *lorem_ipsum = "At vero eos et accusamus et iusto odio "
    "dignissimos ducimus qui blanditiis praesentium voluptatum deleniti "
    "atque corrupti quos dolores et quas molestias excepturi sint occaecati "
    "cupiditate non provident, similique sunt in culpa qui officia deserunt "
    "mollitia animi, id est laborum et dolorum fuga. Et harum quidem rerum "
    "facilis est et expedita distinctio. Nam libero tempore, cum soluta nobis "
    "est eligendi optio cumque nihil impedit quo minus id quod maxime placeat"
    " facere possimus, omnis voluptas assumenda est, omnis dolor repellendus."
    " Temporibus autem quibusdam et aut officiis debitis aut rerum "
    "necessitatibus saepe eveniet ut et voluptates repudiandae sint et "
    "molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente "
    "delectus, ut aut reiciendis voluptatibus maiores alias consequatur "
    "aut perferendis doloribus asperiores repellat.";
    plugin_log *log = plugin_log_create();
    if ( log != NULL )
    {
        int n = 42;
        char *fmt1 = "The answer is %d\n";
        char *fmt2 = "Its name was %s\n";
        char *name = "fred";
        plugin_log_add( log, fmt1, n );
        plugin_log_add( log, fmt2, name );
        if ( plugin_log_pos(log) !=
            strlen(fmt1)
            +strlen(fmt2)
            +strlen(name)
            +(int)log10(n)-3 )
        {
            fprintf(stderr,"plugin_log: failed to copy data\n");
            (*failed)++;
        }
        else
            (*passed)++;
        int old_pos = plugin_log_pos(log);
        plugin_log_add( log, "%s", lorem_ipsum );
        if ( plugin_log_pos(log) != old_pos+127 )
        {
            fprintf(stderr,"plugin_log: failed to copy longer data\n");
            (*failed)++;
        }
        else
            (*passed);
        plugin_log_dispose( log );
        // managed to allocate
        (*passed)++;
    }
    else
        (*failed)++;
}
#endif