#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "bitset.h"
#include "link_node.h"
#include "version.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "plugin_log.h"
#include "encoding.h"
#ifdef MEMWATCH 
#include "memwatch.h"
#endif

/**
 * Sort the version list by version-id using shellsort
 * @param a the array of version objects
 * @param N the number of version objects
 */
static void sort_versions( version **a, int N )
{
    int i, j, k, h; 
    version *v;
    int incs[16] = { 1391376, 463792, 198768, 86961, 33936,
        13776, 4592, 1968, 861, 336, 
        112, 48, 21, 7, 3, 1 };
    for ( k = 0; k < 16; k++)
    {
        for ( h=incs[k],i=h;i<=N-1;i++ )
        { 
            v = a[i]; 
            j = i;
            version *v = a[i];
            UChar *a_id = version_id(a[j-h]);
            UChar *v_id = version_id(v);
            while ( j >= h && u_strcmp(a_id,v_id)>0 )
            { 
                a[j] = a[j-h]; 
                j -= h; 
            }
            a[j] = v; 
        }
    }
}
/**
 * Write a utf-8 string to the destination
 * @param ustr the utf-16 string
 * @param data the destination char buffer
 * @param data_len its length
 * @return 1 if it worked else 0
 */
static int write_utf8_string( UChar *ustr, char *data, int *data_len )
{
    int res = 0;
    int len = measure_to_encoding( ustr, u_strlen(ustr), "utf-8" );
    char *dst = calloc( len+1, 1 );
    if ( dst != NULL )
    {
        int n = convert_to_encoding( ustr, len, dst, len+1, "utf-8" );
        if ( n == len )
        {
            strcat( data, dst );
            *data_len-=strlen(dst);
            res = 1;
        }
        free( dst );
    }
    if ( *data_len > 1 )
    {
        strcat( data, "\n" );
        *data_len--;
    }
    else
        res = 0;
    return res;
}
/**
 * Write a single tab-separated list of version path components in utf-8
 * @param da an array of previous path components if any
 * @param vid the version-id to print
 * @param data the destination utf8-string
 * @param data_len the amount of store left in dst
 * @return 1 if it worked else 0
 */
static int write_line( dyn_array *da, UChar *vid, char *data, int *data_len, 
    plugin_log *log )
{
    int res = 1;
    int len = measure_to_encoding( vid, u_strlen(vid), "utf-8" );
    if ( len+1 < *data_len )
    {
        char *dst = calloc( len+1, 1 );
        if ( dst != NULL )
        {
            int n = convert_to_encoding( vid, len, dst, len+1, "utf-8" );
            if ( n == len )
            {
                int i = 0;
                char *seg = strtok( dst, "/" );
                while ( seg != NULL && res )
                {
                    if ( i<dyn_array_size(da) )
                    {
                        int t_res = 1;
                        char *col = dyn_array_get( da, i );
                        if ( strcmp(col,seg)==0 )
                        {
                            if ( i > 0 )
                                col = "\t";
                            else
                                col = "";
                        }
                        else if ( col != NULL )
                        {
                            free( col );
                            col = strdup(seg);
                            if ( col == NULL )
                                t_res = 0;
                            else
                            {
                                dyn_array_remove( da, i );
                                t_res = dyn_array_insert( da, col, i );
                            }
                        }
                        if ( t_res && *data_len > strlen(col)+1 )
                        {
                            if ( i > 0 && strcmp(col,"\t")!=0 )
                                strcat( data, "\t");
                            strcat( data, col );
                            *data_len-=strlen(col);
                        }
                        else
                            res = 0;
                    }
                    else if ( strlen(seg)+1 < *data_len )
                    {
                        res = dyn_array_add( da, strdup(seg) );
                        if ( res && *data_len > strlen(seg)+1 )
                        {
                            if ( i > 0 )
                                strcat( data, "\t");
                            strcat( data, seg );
                            *data_len-=strlen(seg);
                        }
                        else
                        {
                            plugin_log_add(log,"failed to add seg to string\n");
                            res = 0;
                        }
                    }
                    seg = strtok( NULL, "/" );
                    i++;
                }
            }
            free( dst );
        }
        else
        {
            plugin_log_add(log,"length of version ID too great\n");
            res = 0;
        }
    }
    if ( *data_len > 1 )
    {
        strcat( data, "\n" );
        *data_len--;
    }
    else
    {
        plugin_log_add(log,"len\n");
        res = 0;
    }
    return res;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulte or read
 * @param options a string contianing the plugin's options
 * @param output buffer of length SCRATCH_LEN
 * @param data user-supplied data or NULL
 * @param data_len of data (0 if data is NULL)
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, char **output, 
    unsigned char *data, size_t data_len )
{
    int res = 1;
    if ( mvd != NULL )
    {
        plugin_log *log = plugin_log_create();
        if ( log != NULL )
        {
            int n_versions = mvd_count_versions( *mvd );
            version **versions = calloc( n_versions, sizeof(version*) );
            if ( versions != NULL )
            {
                res = mvd_get_versions( *mvd, versions, n_versions );
                if ( res )
                {
                    int i,o_len = plugin_log_pos(log);
                    dyn_array *da = dyn_array_create( 5 );
                    if ( da != NULL )
                    {
                        UChar *desc = mvd_description( *mvd );
                        sort_versions( versions, n_versions );
                        *output = plugin_log_buffer(log);
                        res = write_utf8_string( desc, *output, &o_len );
                        if ( !res )
                            plugin_log_add(log,
                                "mvd_list: write_utf8_string failed\n");
                        for ( i=0;i<n_versions;i++ )
                        {
                            version *v = versions[i];
                            res = write_line( da, version_id(v), *output, 
                                &o_len, log );
                            if ( !res )
                            {
                                plugin_log_add(log,
                                    "mvd_list: write_line failed. o_len=%d\n",
                                    o_len);
                                break;
                            }
                        }
                        for ( i=0;i<dyn_array_size(da);i++ )
                            free( dyn_array_get(da,i) );
                        dyn_array_dispose( da );
                    }
                    else
                        plugin_log_add(log,"mvd_list: no room for cols\n");
                }
                else
                    plugin_log_add(log,"mvd_list: failed to fetch versions\n");
                free( versions );
            }
            else
                plugin_log_add(log,"mvd_list: failed to retrieve versions\n");
            plugin_log_dispose( log );
        }
    }
    return res;
}
/**
 * Print a help message to stdout explaining what the paramerts are
 * @return a string
 */
char *help()
{
    return "help\n";
}
/**
 * Tell the world what we do
 * @return a string
 */
char *description()
{
    return "list the version IDs and version descriptions in an mvd\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    return "version 0.1 (c) 2013 Desmond Schmidt\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "list";
}
/**
 * Do we change the MVD?
 * @return 1 if we do else 0
 */
int changes()
{
    return 0;
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test(int *p,int *f)
{
    return 1;
}
