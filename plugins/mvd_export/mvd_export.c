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
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "version.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "hashmap.h"
#include "option_keys.h"
#include "plugin.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulte or read
 * @param options a string contianing the plugin's options
 * @param output buffer of length SCRATCH_LEN
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char *output,
    unsigned char *data, size_t data_len )
{
    int res = 1;
    char *file = "default.json";
    hashmap *map = parse_options( options );
    if ( map != NULL && hashmap_contains(map,DEST_FILE_KEY) )
    {
        file = hashmap_get( map, DEST_FILE_KEY );
    }
    res = mvd_json_externalise( *mvd, file, "utf-8" );
    if ( map != NULL )
        hashmap_dispose( map, free );
    return res;
}
int changes()
{
    return 0;
}
/**
 * Print a help message to stdout explaining what the paramerts are
 */
char *help()
{
    return "help\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    "version 0.1 (c) 2013 Desmond Schmidt\n";
}
/**
 * Explain what we do
 * @return a string
 */
char *description()
{
    return "Export an mvd to JSON\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "export";
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
