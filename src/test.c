/* 
 * File:   test.c
 * Author: desmond
 *
 * Created on January 12, 2013, 4:01 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "operation.h"
#include "mvdtool.h"
#include "chunk_state.h"
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "pair.h"
#include "version.h"
#include "mvd.h"
#include "mvdfile.h"
#include "b64.h"
#include "char_buf.h"
#include "dyn_string.h"
#include "dyn_array.h"
#include "hashmap.h"
#include <stdint.h>
#include "hsieh.h"
#include "zip.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
static int total_passed = 0;
static int total_failed = 0;
/*
 * Run all the tests
 */
#ifdef MVD_TEST
typedef void (*test_function)(int *p,int *f);
static void report_test( const char *name, test_function tf, int *p, int *f )
{
    (tf)(p,f);
    fprintf(stderr,"%s: passed %d failed %d tests\n",name,*p,*f );
    total_passed += *p;
    total_failed += *f;
    *p = *f = 0;
}
int main(int argc, char** argv) 
{
    int passed=0;
    int failed=0;
    
    report_test( "bitset", test_bitset, &passed, &failed );
    report_test( "chunk_state", test_chunk_state,&passed,&failed);
    report_test( "mvdtool", test_mvdtool,&passed,&failed);
    report_test( "b64", test_b64,&passed,&failed);
    report_test( "zip", test_zip,&passed,&failed);
    report_test( "dyn_string", test_dyn_string,&passed,&failed);
    report_test( "mvdfile", test_mvdfile,&passed,&failed);
    report_test( "char_buf", test_char_buf,&passed,&failed);
    report_test( "dyn_array", test_dyn_array,&passed,&failed);
    report_test( "dyn_string", test_dyn_string,&passed,&failed);
    report_test( "hashmap", test_hashmap,&passed,&failed );
    report_test( "hsieh", test_hsieh,&passed,&failed );
    report_test( "link_node", test_link_node,&passed,&failed );
    
    fprintf( stdout, "total passed %d failed %d tests\n",total_passed,
        total_failed);
}
#endif
