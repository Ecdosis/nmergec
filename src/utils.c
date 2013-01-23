#include <stdio.h>
#include "utils.h"
// even 64-bit unsigned is just 20 bytes long as text
#define ITOA_MAXLEN 32
static char itoa_scratch[ITOA_MAXLEN];
/**
 * convert an integer into a temporary string
 * @param value the int value to stringify
 * @return a temporary string (valid until next call to itoa)
 */
char *itoa( int value )
{
    snprintf(itoa_scratch,ITOA_MAXLEN,"%d",value);
    return itoa_scratch;
}

