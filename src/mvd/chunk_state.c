#include <stdio.h>
#include <string.h>
#include "mvd/chunk_state.h"
static const char *table[] = {"a_chunk","added","backup","child",
"deleted","found","merged","parent","partial" };
/**
 * Convert a string to uppercase
 * @param str
 */
static void str_to_lower( char *str )
{
    int i,len = strlen( str );
    for ( i=0;i<len;i++ )
        str[i] = tolower(str[i]);
}
/**
 * Convert a command name into an actual command enum value
 * @param value the value to convert
 * @return an enum value
 */
chunk_state chunk_state_value( const char *value )
{
    int N = (int)sizeof(table)/sizeof(const char*);
    char *vcopy = strdup(value);
    if ( vcopy != NULL )
    {
        int top = 0;
        int bottom = N-1;
        str_to_lower( vcopy );
        while ( top <= bottom )
        {
            int mid = (top+bottom)/2;
            int res = strcmp(vcopy,table[mid]);
            if ( res==0 )
                return (chunk_state)mid;
            else if ( res < 0 )
                bottom = mid-1;
            else
                top = mid+1;
        }
        free( vcopy );
    }
    else
        fprintf(stderr,"chunk_state: failed to copy string %s\n",value);
    return a_chunk;
}
#ifdef MVD_TEST
static void test_c( const char *value, chunk_state answer, int *p, int *f )
{
    chunk_state c = chunk_state_value( value );
    if ( c == answer )
        (*p)++;
    else
    {
        fprintf(stderr,"failed to convert %s chunk_state, index=%d\n",value,(int)c);
        (*f)++;
    }
}
void test_chunk_state( int *passed, int *failed )
{
    int chunk_passed = 0;
    int chunk_failed = 0;
    test_c( "a_chunk", a_chunk, &chunk_passed, &chunk_failed );
    test_c( "added", added, &chunk_passed, &chunk_failed );
    test_c( "backup", backup, &chunk_passed, &chunk_failed );
    test_c( "child", child, &chunk_passed, &chunk_failed );
    test_c( "deleted", deleted, &chunk_passed, &chunk_failed );
    test_c( "found", found, &chunk_passed, &chunk_failed );
    test_c( "merged", merged, &chunk_passed, &chunk_failed );
    test_c( "parent", parent, &chunk_passed, &chunk_failed );
    test_c( "partial", partial, &chunk_passed, &chunk_failed );
    *passed += chunk_passed;
    *failed += chunk_failed;
}
#endif
