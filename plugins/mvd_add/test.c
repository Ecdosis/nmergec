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
#ifdef MVD_TEST
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "version.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "hashmap.h"
#include "hashtable.h"
#include "utils.h"
#include "aatree.h"
#include "path.h"
#include "linkpair.h"
#include "orphanage.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
static int total_passed = 0;
static int total_failed = 0;

                    
/**
 * Get the length of an open file
 * @param fp an open FILE handle
 * @return file length if successful, else 0
 */
static int file_length( FILE *fp )
{
	int length = 0;
    int res = fseek( fp, 0, SEEK_END );
	if ( res == 0 )
	{
		long long_len = ftell( fp );
		if ( long_len > INT_MAX )
        {
			fprintf( stderr,"mvd_add: file too long: %ld", long_len );
            length = res = 0;
        }
		else
        {
            length = (int) long_len;
            if ( length != -1 )
                res = fseek( fp, 0, SEEK_SET );
            else
                res = 1;
        }
	}
	if ( res != 0 )
    {
		fprintf(stderr, "mvd_add: failed to read file. error %s\n",
            strerror(errno) );
        length = 0;
    }
	return length;
}
int write_mvd( MVD *mvd, char *file )
{
    int res = 0;
    int size = mvd_datasize( mvd, 1 );
    unsigned char *data = malloc( size );
    if ( data != NULL )
    {
        int res = mvd_serialise( mvd, data, size, 1 );
        if ( res )
        {
            FILE *dst = fopen( file, "w" );
            if ( dst != NULL )
            {
                int nitems = fwrite( data, 1, size, dst );
                if ( nitems == size )
                    res = 1;
                fclose( dst );
            }
        }
    }
    return res;
}
typedef void (*test_function)(int *p,int *f);
static void report_test( const char *name, test_function tf, int *p, int *f )
{
    (tf)(p,f);
    fprintf(stderr,"%s: passed %d failed %d tests\n",name,*p,*f );
    total_passed += *p;
    total_failed += *f;
    *p = *f = 0;
}
int main( int argc, char **argv )
{

    int passed=0;
    int failed=0;
    // reinstate this after debugging
    /*report_test( "aatree", aatree_test,&passed,&failed);
    report_test( "node", node_test,&passed,&failed);
    report_test( "hashtable", hashtable_test,&passed,&failed);
    report_test( "suffixtree", suffixtree_test,&passed,&failed);
    report_test( "plugin_log", plugin_log_test,&passed,&failed);
    report_test( "path", path_test,&passed,&failed);
    report_test( "linkpair", linkpair_test,&passed,&failed);
    report_test( "orphanage", orphanage_test,&passed,&failed);
    
    fprintf( stdout, "total passed %d failed %d tests\n",total_passed,
        total_failed);
*/
    test_mvd_add( &passed, &failed );
}
#endif
