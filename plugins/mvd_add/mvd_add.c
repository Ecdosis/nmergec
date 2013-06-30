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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "bitset.h"
#include "link_node.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "pair.h"
#include "version.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "encoding.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "hashmap.h"
#include "utils.h"
#include "option_keys.h"
#include "linkpair.h"
#include "match.h"
#include "aatree.h"
#include "alignment.h"
#include "matcher.h"
#include "verify.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define ALIGNMENTS_LEN 100 

// object to store state during a call to process
struct add_struct
{
    // the version ID - a /-delimited string
    UChar *vid;
    // description of the version to be added
    UChar *v_description;
    // MVD description - if need to create one
    UChar *mvd_description;
    // the text itself - we use wide UTF-16 as the basic char type
    UChar *text;
    // number of wchar_t characters in text 
    size_t tlen;
    // encoding of the input file
    char *encoding;
};

/**
 * Convert an ascii string to unicode using the given encoding
 * @param src an 8-bit plain string
 * @param src_len the length of src in BYTES
 * @param encoding the encoding's canonical name
 * @param dst_len VAR param to whole number of UChars converted into
 * @param log a plugin log to record errors
 * @return an allocated UChar string or NULL
 */
static UChar *ascii_to_unicode( char *src, int src_len, char *encoding, 
    int *dst_len, plugin_log *log )
{
    int u_len = measure_from_encoding( src, src_len, encoding );
    UChar *dst = calloc( u_len+1, sizeof(UChar) );
    if ( dst != NULL )
    {
        int nchars = convert_from_encoding( src, src_len,
            dst,u_len+1,encoding);
        if ( nchars == 0 )
        {
            plugin_log_add(log,
                "mvd_add: failed to convert %s via encoding %s\n",
                src, encoding);
            free( dst );
            dst = NULL;
        }
        else
            *dst_len = nchars/sizeof(UChar);
    }
    else
        plugin_log_add(log,"mvd_add: failed to allocate UChar buffer\n");
    return dst;
}
/**
 * Parse the options
 * @param map of key-value pairs already parsed
 * @param log the log to record errors in
 * @return 1 if the options were sane
 */
static int set_options( struct add_struct *add, hashmap *map, plugin_log *log )
{
    int sane = 1;
    char *value;
    if ( hashmap_contains(map,ENCODING_KEY) )
        add->encoding = hashmap_get(map,ENCODING_KEY);
    if ( hashmap_contains(map,VID_KEY) )
    {
        int tlen;
        value = hashmap_get(map,VID_KEY);
        add->vid = ascii_to_unicode( value, strlen(value), add->encoding, 
            &tlen, log );
        if ( add->vid == NULL )
            sane = 0;
    }
    else
    {
        plugin_log_add(log,"mvd_add: missing version ID\n");
        sane = 0;
    }
    if ( hashmap_contains(map,LONG_NAME_KEY) )
    {
        value = hashmap_get(map,LONG_NAME_KEY);
        int tlen;
        add->v_description = ascii_to_unicode(value,strlen(value),
            add->encoding,&tlen,log );
        if ( add->v_description == NULL )
        {
            sane = 0;
            plugin_log_add(log,"mvd_add: missing version description\n");
        }
    }
    else if ( add->vid != NULL )
    {
        add->v_description = DEFAULT_LONG_NAME;
    }
    if ( hashmap_contains(map,DESCRIPTION_KEY) )
    {
        value = hashmap_get(map,DESCRIPTION_KEY);
        int tlen;
        add->mvd_description = ascii_to_unicode(value,strlen(value),
            add->encoding,&tlen,log );
        if ( add->mvd_description == NULL )
        {
            sane = 0;
            plugin_log_add(log,"mvd_add: missing mvd description\n");
        }
    }
    else
        add->mvd_description = u_strdup(DEFAULT_DESCRIPTION);
    return sane;
}
/**
 * Add the first version to an mvd
 * @param mvd the mvd to add it to
 * @param add the object representing this mvd_add operation
 * @param text the unicode text that forms the version
 * @param tlen the length in UChars
 * @param log the log to record errors in
 * @return 1 if it worked
 */ 
static int add_first_version( MVD *mvd, struct add_struct *add, UChar *text, 
    int tlen, plugin_log *log )
{
    int res = 1;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, 1 );
        pair *p = pair_create_basic( bs, text, tlen );
        res = mvd_add_pair( mvd, p );
        if ( !res )
            plugin_log_add(log,
            "mvd_add: failed to add first version\n");
        else
        {
            version *v = version_create( add->vid, add->v_description );
            if ( v != NULL )
                res = mvd_add_version( mvd, v );
            if ( res == 0 )
                plugin_log_add(log,
                "mvd_add: failed to add version\n");
            else
                plugin_log_add(log,"mvd_add: added version successfully!\n");
        }
    }
    return res;
}
/**
 * Get the longest match of 3 but favour the 2nd
 * @param a the first match or NULL
 * @param b the second match of NULL
 * @param c the third match or NULL
 * @return the longest of the three or b
 */
match *best_of_three( match *a, match *b, match *c )
{
    match *maxAC = (match_total_len(a)>match_total_len(c))?a:c;
    return (match_total_len(maxAC)>match_total_len(b))?maxAC:b;
}
/**
 * Wrap an array of pairs into a doubly linked list
 * @param pairs a pairs list as a dyn_array
 * @param log the log to add it to
 * @return head of the linkpairs list
 */
static linkpair *link_pairs( dyn_array *pairs, plugin_log *log )
{
    int len = dyn_array_size( pairs );
    int i;
    linkpair *head = NULL;
    linkpair *current;
    for ( i=0;i<len;i++ )
    {
        pair *p = (pair*)dyn_array_get( pairs, i );
        linkpair *lp = linkpair_create( p, log );
        if ( lp != NULL )
        {
            if ( head == NULL )
                current = head = lp;
            else
            {
                linkpair_set_right( current, lp );
                linkpair_set_left( lp, current );
                current = lp;
            }
        }
        else
            break;
    }
    return head;
}
/**
 * Add a version to an MVD that already has at least one
 * @param mvd the mvd to add it to
 * @param add the mvd_add object containing options
 * @param text the text to add
 * @param tlen the length of text in UChars
 * @param log the log to record errors in
 * @return 1 if it worked
 */
static int add_subsequent_version( MVD *mvd, struct add_struct *add, 
    UChar *text, int tlen, plugin_log *log )
{
    int res = 1;
    alignment *head = NULL;
    // create initial alignment
    alignment *a = alignment_create( text, 0, tlen, mvd_count_versions(mvd)+1, 
        log );
    if ( a != NULL )
    {
        head = a;
        while ( res && head != NULL )
        {
            alignment *left,*right;
            dyn_array *pairs = mvd_get_pairs(mvd);
            linkpair *pairs_list = link_pairs(pairs,log);
            // add new pair to the start
            linkpair *start = alignment_linkpair( a );
            linkpair_set_right( start, pairs_list );
            linkpair_set_left( pairs_list, start );
            pairs_list = start;
            // add its version to the mvd
            version *new_v = version_create( add->vid, add->v_description );
            if ( new_v != NULL )
                res = mvd_add_version( mvd, new_v );
            if ( res )
            {
                // this does all the work
                res = alignment_align( head, &pairs_list, &left, &right, log );
                if ( res )
                {
                    // for debugging we do this now but normally at the end
                    pairs = linkpair_to_pairs( pairs_list );
                    mvd_set_pairs( mvd, pairs );
                    verify *v = verify_create( pairs, mvd_count_versions(mvd) );
                    if ( !verify_check(v) )
                        fprintf(stderr,"error: unbalanced graph\n");
                    // temporary
                    break;
                    // end debug
                    head = alignment_pop( head );
                    // now update the list
                    if ( head != NULL )
                    {
                        if ( left != NULL )
                            alignment_push( head, left );
                        if ( right != NULL )
                            alignment_push( head, right );
                    }
                    else if ( left != NULL )    // head,tail is NULL
                    {
                        head = left;
                        if ( right != NULL )
                            alignment_push( head, right );
                    }
                    else if ( right != NULL )   // left is NULL
                        head = right;
                }
                else
                    head = alignment_pop(head);
            }
        }
    }
    return res;
}
/**
 * Add the supplied text to the MVD
 * @param add the add struct initialised and ready to go
 * @param mvd the MVD to add it to
 * @return 1 if it worked else 0
 */
static int add_mvd_text( struct add_struct *add, MVD *mvd, 
    unsigned char *data, size_t data_len, plugin_log *log )
{
    int res = 1;
    int tlen;
    char *mvd_encoding = mvd_get_encoding(mvd);
    if ( add->encoding != NULL )
    {
        if ( mvd_count_versions(mvd)==0 )
            mvd_set_encoding(mvd,add->encoding);
        else if ( strcmp(mvd_encoding,add->encoding)!=0 )
        {
            plugin_log_add(log,
                "file's encoding %s does not match mvd's (%s):"
                " assimilating...\n",add->encoding,mvd_encoding);
            mvd_encoding = add->encoding;
        }
    }
    // else use the mvd's current encoding
    // convert from the external encoding to UTF-16 
    UChar *text = ascii_to_unicode( data, data_len, 
        mvd_encoding, &tlen, log );
    if ( tlen > 0 )
    {
        if ( mvd_count_versions(mvd)==0 )
        {
            mvd_set_description( mvd, add->mvd_description );
            res = add_first_version( mvd, add, text, tlen, log );
        }
        else
            res = add_subsequent_version( mvd, add, text, tlen, log );
    }
    else
    {
        plugin_log_add(log,"mvd_add: text is empty\n");
        res = 0;
    }
    return res;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to manipulate or read
 * @param options a string containing the plugin's options
 * @param output buffer of length SCRATCH_LEN
 * @param data data passed directly in 
 * @param data_len length of data
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char *output, 
    unsigned char *data, size_t data_len )
{
    int res = 1;
    if ( *mvd == NULL )
        *mvd = mvd_create( 1 );
    if ( *mvd != NULL )
    {
        plugin_log *log = plugin_log_create( output );
        if ( log != NULL )
        {
            if ( !mvd_is_clean(*mvd) )
                res = mvd_clean( *mvd );
            if ( res )
            {
                struct add_struct *add = calloc( 1, sizeof(struct add_struct) );
                if ( add != NULL )
                {
                    hashmap *map = parse_options( options );
                    if ( map != NULL )
                    {
                        res = set_options( add, map, log );
                        if ( res && data != NULL && data_len > 0 )
                            res = add_mvd_text( add, *mvd, data, data_len, 
                                log );
                        else
                            plugin_log_add(log,"mvd_add: length was 0\n");
                        hashmap_dispose( map, free );
                    }
                    else
                        plugin_log_add(log,"mvd_add: invalid options %s",
                            options);
                    free( add );
                }
                else
                    plugin_log_add(log,
                        "mvd_add: failed to create mvd_add object\n");
                plugin_log_dispose( log );
            }
            else
            {
                plugin_log_add(log,"mvd failed to clean\n");
            }
        }
        else
        {
            plugin_log_add(log,"mvd_add: failed to create log object\n");
            res = 0;
        }
    }
    return res;
}
/**
 * Print a help message to stdout explaining what the parameters are
 */
char *help()
{
    return "options:\n\t"
        "text=<file>: file to add to existing MVD (optional)\n\t"
        "vid=<vid>: version ID as a /-delimited path\n\t"
        "encoding=<ascii-encoding-name>: the text encoding\n\t"
        "description=<string>: description of new version in encoding\n\t"
        "debug=1|0: turn debugging on (default off)\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    return "version 1.0 (c) 2013 Desmond Schmidt\n";
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "add";
}
/**
 * Do we change the MVD?
 * @return 1 if we do else 0
 */
int changes()
{
    return 1;
}
/**
 * Provide a description of this plugin
 * @return a string
 */
char *description()
{
    return "adds a new version to an existing mvd\n";
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test( int *p, int *f )
{
    return 1;
}
