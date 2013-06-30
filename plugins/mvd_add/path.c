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
#include "plugin_log.h"
#include "path.h"
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
char path_first( path *p, char *str )
{
    return str[p->start];
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

