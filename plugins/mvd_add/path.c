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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "path.h"
#include "encoding.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
// for storing gamma (walking across the tree)
struct path_struct
{
    int start;
    int len;
};
/**
 * Create a path
 * @param start the start index into str
 * @param len the length of this path 
 * @param log the log to record errors in
 * @return the complete path
 */
path *path_create( int start, int len, plugin_log *log )
{
    path *p = calloc( 1, sizeof(path) );
    if ( p == NULL )
        plugin_log_add(log,"failed to create path\n");
    else
    {
        p->start = start;
        p->len = len;
    }
    return p;
}
/**
 * Add one path to the front of another
 * @param p the current path
 * @param len the length of the prefix
 */
void path_prepend( path *p, int len )
{
    p->start -= len;
    p->len += len;
}
/**
 * Get the first char in the path
 * @param p the path in question
 * @param str the string it refers to
 * @return the first char
 */
UChar path_first( path *p, UChar *ustr )
{
    return ustr[p->start];
}
/**
 * Dispose of a path cleanly
 * @param p the path to dispose
 */
void path_dispose( path *p )
{
    free( p );
}
/**
 * Access the start field
 * @param p the path in question
 * @return the path start index in str
 */
int path_start( path *p )
{
    return p->start;
}
/**
 * Access the start field
 * @param p the path in question
 * @return the path length
 */
int path_len( path *p )
{
    return p->len;
}
#ifdef MVD_TEST
static char *cstr = "Lorem ipsum dolor sit amet, consectetur";
static char buffer[SCRATCH_LEN];
void path_test( int *passed, int *failed )
{
    plugin_log *log = plugin_log_create( buffer );
    if ( log != NULL )
    {
        path *p = path_create( 23, 5, log );
        if ( path_len(p)==5 )
            (*passed)++;
        else
        {
            fprintf(stderr,"path: incorrect length\n");
            (*failed)++;
        }
        path_prepend(p,11);
        if ( path_start(p)==12 )
            (*passed)++;
        else
        {
            fprintf(stderr,"path: prepend failed\n");
            (*failed)++;
        }
        if ( path_len(p)!=35 )
            (*passed)++;
        else
        {
            fprintf(stderr,"path: path length wrong\n");
            (*failed)++;
        }
        int clen = strlen(cstr);
        int ulen = measure_from_encoding( cstr, clen, "utf-8" );
        if ( ulen > 0 )
        {
            UChar *dst = calloc( ulen+1, sizeof(UChar) );
            if ( dst != NULL )
            {
                int res = convert_from_encoding( cstr, clen, dst, 
                    ulen+1, "utf-8" );
                if ( res )
                {
                    UChar uc = path_first(p,dst);
                    if ( uc != (UChar)'d' )
                    {
                        (*failed)++;
                        fprintf(stderr,"path: failed to index into string\n");
                    }
                    else
                        (*passed)++;
                }
                free( dst );
            }
        }
        plugin_log_dispose( log );
    }
    else
    {
        (*failed)++;
    }
}
#endif