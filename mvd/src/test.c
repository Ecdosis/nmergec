/* 
 * File:   test.c
 * Author: desmond
 *
 * Created on January 12, 2013, 4:01 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "unicode/uchar.h"
#include "version.h"
#include "dyn_array.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "mvd.h"
#include "encoding.h"
#include "group.h"
#include "hsieh.h"
#include "hashmap.h"
#include "vgnode.h"
#include "hint.h"
#include "utils.h"
#include "serialiser.h"
#include "util.h"
#include "version.h"
#include "vgnode.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
static int total_passed = 0;
static int total_failed = 0;
/*
 * Run all the tests
 */
#ifdef MVD_TEST
// just for testing!
//#include "mvdfile.h"
typedef void (*test_function)(int *p,int *f);
static void report_test( const char *name, test_function tf, int *p, int *f )
{
    (tf)(p,f);
    fprintf(stderr,"%s: passed %d failed %d tests\n",name,*p,*f );
    total_passed += *p;
    total_failed += *f;
    *p = *f = 0;
}
/*
static void do_mvd_test( int *passed, int *failed )
{
    MVD *mvd = mvdfile_load( "tests/kinglear.mvd" ); 
    if ( mvd != NULL )
    {
        test_mvd( mvd, passed, failed );
        mvd_dispose( mvd );
    }
}
*/
int main(int argc, char** argv) 
{
    int passed=0;
    int failed=0;
    
    // test mvd library
    report_test( "bitset", test_bitset,&passed,&failed);
    report_test( "link_node", test_link_node, &passed, &failed );
    report_test( "dyn_array", test_dyn_array, &passed, &failed );
    report_test( "encoding", test_encoding, &passed, &failed );
    report_test( "group", test_group, &passed, &failed );
    report_test( "hashmap", test_hashmap, &passed, &failed );
    report_test( "hint", test_hint, &passed, &failed );
    report_test( "hsieh", test_hsieh, &passed, &failed );
    report_test( "link_node", test_link_node, &passed, &failed );
    //do_mvd_test( &passed, &failed );
    report_test( "pair", test_pair, &passed, &failed );
    report_test( "serialiser", test_serialiser, &passed, &failed );
    report_test( "utils", test_utils, &passed, &failed );
    report_test( "version", test_version, &passed, &failed );
    report_test( "vgnode", test_vgnode, &passed, &failed );
    fprintf( stdout, "total passed %d failed %d tests overall\n",
        total_passed, total_failed);
}
#endif

