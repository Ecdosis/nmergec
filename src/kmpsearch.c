/*
 *  NMerge is Copyright 2009 Desmond Schmidt
 * 
 *  This file is part of NMerge. NMerge is a Java library for merging 
 *  multiple versions into multi-version documents (MVDs), and for 
 *  reading, searching and comparing them.
 *
 *  NMerge is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kmpsearch.h"
#ifdef MVD_DEBUG
#include "memwatch.h"
#endif

/**
 * Search a byte array fairly efficiently. 
 * @author Desmond Schmidt 1/6/09
 */
static int n = 1;
/**
 * Initialise the next table
 * @param pattern the pattern as a byte array in any encoding
 * @return an array of next indices
 */
static int *initNext( char *pattern ) 
{
    int plen = strlen(pattern);
    int *next = calloc(plen,sizeof(int));
    if ( next != NULL )
    {
        int i = 0, j = -1;
        next[0] = -1;
        while (i < plen - 1) 
        {
            while ( j >= 0 && pattern[i] != pattern[j] )
                j = next[j];
            i++; j++;
            next[i] = j;
        }
    }
    else
        fprintf(stderr,"kmpsearch: failed to allocate next array\n");
    return next;
}
/**
 * Perform the search
 * @param text the byte array to search in
 * @param offset within text from which to search
 * @param pattern the byte array of the pattern
 * @return the index into text where the pattern occurs or -1
 */
int search( char *text, int offset, char *pattern ) 
{
    int tlen = strlen( text );
    int plen = strlen( pattern );
    int *next = initNext( pattern );
    if ( next != NULL )
    {
        int i = offset, j = 0;
        n = 1;
        while ( i < tlen ) 
        {
            while ( j >= 0 && pattern[j] != text[i] ) 
            {
                j = next[j];
            }
            i++; 
            j++;
            if ( j == plen )
            {
                free( next );
                return i - plen;
            }
        }
        free( next );
    }
    return -1;
}

#ifdef MVD_DEBUG
/**
 * Test the kmpsearch routine
 * @param passed VAR param for number of tests passed
 * @param failed VAR param for number of tests failed
 * @return 1 if all tests passedelse 0
 */
int test_kmpsearch( int *passed, int *failed )
{
    *passed = 0;
    *failed = 0;
    const char *text = "Lorem ipsum dolor sit amet, consectetur adipisicing "
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
    " Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi "
    "ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
    "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa "
    "qui officia deserunt mollit anim id est laborum.";
    int res = search( (char*)text, 0, "nostrud");
    if ( res != -1 )
        (*passed)++;
    else
    {
        (*failed)++;
        printf("could not find nostrud\n");
    }
    res = search( (char*)text, 0, "qui officia deserunt");
    if ( res != -1 )
        passed++;
    else
    {
        (*failed)++;
        printf("could not find qui officia deserunt\n");
    }
    res = search( (char*)text, 0, "anim id est laborum.");
    if ( res != -1 )
        (*passed)++;
    else
    {
        (*failed)++;
        printf("could not find anim id est laborum. (at text end)\n");
    }
    res = search( (char*)text, 0, "Lorem ipsum");
    if ( res != -1 )
        (*passed)++;
    else
    {
        (*failed)++;
        printf("could not find Lorem ipsum (at text start)\n");
    }
    res = search( (char*)text, 0, "bananarama");
    if ( res != -1 )
    {
        (*failed)++;
        printf("found bananarama at %d (but shouldn't have!)\n",res);
    }
    else
        (*passed)++;
    res = search( (char*)text, 0, "exercitaton");
    if ( res != -1 )
    {
        (*failed)++;
        printf("found exercitaton at %d (but shouldn't have!)\n",res);
    }
    else
        (*passed)++;
    return (*failed)==0;
}
#endif