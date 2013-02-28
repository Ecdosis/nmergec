#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "operation.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
static const char *table[] = {"EMPTY", "HELP", "LIST", "RUN", "VERSION" };
/**
 * Convert a string to uppercase
 * @param str
 */
static void str_to_upper( char *str )
{
    int i,len = strlen( str );
    for ( i=0;i<len;i++ )
        str[i] = toupper(str[i]);
}
/**
 * Convert an operation name into an actual operation enum value
 * @param value the value to convert
 * @return an operation enum value
 */
operation operation_value( const char *value )
{
    int N = (int)sizeof(table)/sizeof(const char*);
    char *vcopy = strdup(value);
    if ( vcopy != NULL )
    {
        int top = 0;
        int bottom = N-1;
        str_to_upper( vcopy );
        while ( top <= bottom )
        {
            int mid = (top+bottom)/2;
            int res = strcmp(vcopy,table[mid]);
            if ( res==0 )
                return (operation)mid;
            else if ( res < 0 )
                bottom = mid-1;
            else
                top = mid+1;
        }
        free( vcopy );
    }
    else
        fprintf(stderr,"operation: failed to copy string %s\n",value);
    return EMPTY;
}
#ifdef MVD_TEST
static void test_op( const char *value, operation answer, int *p, int *f )
{
    operation op = operation_value( value );
    if ( op == answer )
        (*p)++;
    else
    {
        fprintf(stderr,"failed to convert %s operation, index=%d\n",
            value,(int)op);
        (*f)++;
    }
}
void test_operation( int *passed, int *failed )
{
    int op_passed = 0;
    int op_failed = 0;
    test_op( "EMPTY", EMPTY, &op_passed, &op_failed );
    test_op( "ADD", HELP, &op_passed, &op_failed );
    test_op( "LIST", LIST, &op_passed, &op_failed );
    test_op( "RUN", RUN, &op_passed, &op_failed );
    test_op( "VERSION", VERSION, &op_passed, &op_failed );
    *passed += op_passed;
    *failed += op_failed;
}
#endif