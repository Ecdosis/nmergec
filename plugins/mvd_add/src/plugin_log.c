
#include <stdarg.h>
#include <string.h>
#include "plugin_log.h"
#define SCRATCH_LEN 512
// message buffer
static char scratch[SCRATCH_LEN] = {0};
// message buffer len
static int scratch_pos = 0;
void plugin_log( char *fmt, ... )
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
        strncat( scratch, str, SCRATCH_LEN-scratch_pos );
        scratch_pos = strlen( scratch );
    }
    else
    {
        strncat( scratch, fmt, SCRATCH_LEN-scratch_pos );
        scratch_pos = strlen( scratch );
    }
    va_end( ap );
}
void plugin_log_clear()
{
    scratch_pos = 0;
    scratch[0] = 0;
}
char *plugin_log_buffer()
{
    return scratch;
}