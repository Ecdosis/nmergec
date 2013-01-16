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
#include "mvd/chunk_state.h"
#include "b64.h"
#include "char_buf.h"
#include "zip.h"
static int total_passed = 0;
static int total_failed = 0;
/*
 * Run all the tests
 */
#ifdef MVD_TEST
typedef int (*test_function)(int *p,int *f);
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
    
    report_test( "command", test_command,&passed,&failed);
    report_test( "kmpsearch", test_kmpsearch,&passed,&failed);
    report_test( "chunk_state", test_chunk_state,&passed,&failed);
    report_test( "mvdtool", test_mvdtool,&passed,&failed);
    report_test( "b64", test_b64,&passed,&failed);
    report_test( "zip", test_zip,&passed,&failed);
    fprintf( stdout, "total passed %d failed %d tests\n",total_passed,
        total_failed);
}
#endif
