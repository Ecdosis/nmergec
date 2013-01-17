#include <stdio.h>
#include "mvd.h"
#include "plugin.h"

/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulte or read
 * @param options a string contianing the plugin's options
 * @param output VAR param set to NULL if not needed else the output
 * @return 1 if the process completed successfully
 */
int process( MVD *mvd, char *options, unsigned char **output )
{
    return 1;
}
/**
 * Print a help message to stdout explaining what the paramerts are
 */
void help()
{
    printf("help\n");
}
/**
 * Report the plugin's version and author to stdout
 */
void version()
{
    printf( "version 0.1 (c) 2013 Desmond Schmidt\n");
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "compare versions";
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