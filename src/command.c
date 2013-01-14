#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#ifdef MVD_TEST
#include <stdio.h>
#endif
static const char *table[] = {"ACOMMAND", "ADD","ARCHIVE","COMPARE",
	"CREATE","DELETE", "DESCRIPTION", "DETAILED_USAGE", "EXPORT", 
    "FIND", "HELP", "IMPORT", "LIST", "READ", "TREE", "UNARCHIVE", 
	"UPDATE","USAGE", "VARIANTS" };
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
 * Convert a command name into an actual command enum value
 * @param value the value to convert
 * @return an enum value
 */
command command_value( const char *value )
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
                return (command)mid;
            else if ( res < 0 )
                bottom = mid-1;
            else
                top = mid+1;
        }
        free( vcopy );
    }
    else
        fprintf(stderr,"command: failed to copy string %s\n",value);
    return ACOMMAND;
}
#ifdef MVD_TEST
static void test_c( const char *value, command answer, int *p, int *f )
{
    command c = command_value( value );
    if ( c == answer )
        (*p)++;
    else
    {
        fprintf(stderr,"failed to convert %s command, index=%d\n",value,(int)c);
        (*f)++;
    }
}
int test_command( int *passed, int *failed )
{
    int cmd_passed = 0;
    int cmd_failed = 0;
    test_c( "ACOMMAND", ACOMMAND, &cmd_passed, &cmd_failed );
    test_c( "ADD", ADD, &cmd_passed, &cmd_failed );
    test_c( "add", ADD, &cmd_passed, &cmd_failed );
    test_c( "ARCHIVE", ARCHIVE, &cmd_passed, &cmd_failed );
    test_c( "COMPARE", COMPARE, &cmd_passed, &cmd_failed );
    test_c( "CREATE", CREATE, &cmd_passed, &cmd_failed );
    test_c( "DELETE", DELETE, &cmd_passed, &cmd_failed );
    test_c( "DESCRIPTION", DESCRIPTION, &cmd_passed, &cmd_failed );
    test_c( "DETAILED_USAGE", DETAILED_USAGE, &cmd_passed, &cmd_failed );
    test_c( "EXPORT", EXPORT, &cmd_passed, &cmd_failed );
    test_c( "export", EXPORT, &cmd_passed, &cmd_failed );
    test_c( "FIND", FIND, &cmd_passed, &cmd_failed );
    test_c( "HELP", HELP, &cmd_passed, &cmd_failed );
    test_c( "IMPORT", IMPORT, &cmd_passed, &cmd_failed );
    test_c( "LIST", LIST, &cmd_passed, &cmd_failed );
    test_c( "READ", READ, &cmd_passed, &cmd_failed );
    test_c( "TREE", TREE, &cmd_passed, &cmd_failed );
    test_c( "UNARCHIVE", UNARCHIVE, &cmd_passed, &cmd_failed );
    test_c( "unarchive", UNARCHIVE, &cmd_passed, &cmd_failed );
    test_c( "UPDATE", UPDATE, &cmd_passed, &cmd_failed );
    test_c( "USAGE", USAGE, &cmd_passed, &cmd_failed );
    test_c( "VARIANTS", VARIANTS, &cmd_passed, &cmd_failed );
    *passed += cmd_passed;
    *failed += cmd_failed;
    return cmd_failed==0;
}
#endif