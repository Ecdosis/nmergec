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
#include <stdio.h>
#include <stdlib.h>
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "version.h"
#include "MVD.h"
#include "dom.h"
#include "mvd_json.h"
#include "link_node.h"
#include "hashmap.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif

/**
 * Provide a simple JSON format for loading and saving an MVD. 
 * This only works if the content is plain text or JSON itself.
 * We will escape angle brackets and ampersands into entities.
 * Structure: { "mvd": { "description":...,"versions":...
 * "pairs": ...
 */
/**
 * Convert a UTF-16 string to UTF-8
 * @param ustr the original string
 * @param ulen the length in characters
 * @return an allocated utf8-string the user must free
 */
static char *to_utf8( UChar *ustr, int ulen )
{
    int len = measure_to_encoding( ustr, ulen, "utf-8" );
    char *dst = calloc( len+1, sizeof(char) );
    if ( dst != NULL )
    {
        convert_to_encoding( ustr, ulen, dst, len+1, "utf-8" );
    }
    return dst;
}
/**
 * Write one version element to disk
 * @param vparent the parent versions element
 * @param v the version to write
 * @param index the index of the version (>=1)
 * @return 1 if it worked else 0
 */
static int write_one_version( dom_item *vparent, version *v, int index )
{
    int res = 1;
    char istr[8];
    snprintf( istr,8,"%d",index);
    dom_item *velement = dom_object_create( "version" );
    if ( velement != NULL )
    {
        dom_attribute *attr = dom_attribute_create("id",istr);
        if ( attr != NULL )
        {
            res = dom_add_attribute( velement, attr );
            if ( res )
            {
                char *vid = to_utf8( version_id(v), 
                    u_strlen(version_id(v)) );
                if ( vid != NULL )
                {
                    UChar *v_desc = version_description(v);
                    char *description = to_utf8( v_desc, u_strlen(v_desc) );
                    if ( description != NULL )
                    {
                        res = dom_add_attribute( velement, 
                            dom_attribute_create("vid",vid) );
                        if ( res )
                            res = dom_add_attribute( velement, 
                                dom_attribute_create("description",
                                description) );
                        if ( res )
                            res = dom_add_child( vparent, velement );
                        free( description );
                    }
                    free( vid );
                }
                else
                    res = 0;
            }
            else
                res = 0;
        }
        else
            res = 0;
    }
    else
        res = 0;
    return res;
}
/**
 * Write the version definitions out
 * @param dom the document to add them to
 * @param mvd the mvd to get them from
 */
static int write_versions( dom_item *root, MVD *mvd )
{
    int res = 1;
    dom_item *v_parent = dom_array_create( "versions" );
    if ( v_parent != NULL )
    {
        int i,nversions = mvd_count_versions( mvd );
        version **versions = calloc( nversions,sizeof(version*));
        if ( versions != NULL )
        {
            res = mvd_get_versions( mvd, versions, nversions );
            if ( res )
            {
                for ( i=0;i<nversions;i++ )
                {
                    res = write_one_version( v_parent, versions[i], i+1 );
                    if ( !res )
                        break;
                }
            }
        }
        else
            res = 0;
    }
    if ( res )
        res = dom_add_child( root, v_parent );
    return res;
}
/**
 * Add one pair to the dom
 * @param p the pair to add
 * @param p_parent the pair parent ("pairs")
 * @param parents a hashmap of parents looking for children
 * @param orphans a hashmap of children looking for parents
 * @param id VAR param: parent ID to update
 * @return 1 if it worked OK else 0
 */
int write_one_pair( pair *p, dom_item *p_parent, hashmap *parents, 
    hashmap *orphans, int *id )
{
    int res = 1;
    dom_item *p_element = dom_object_create( "pair" );
    if ( p_element != NULL )
    {
        if ( pair_is_parent(p) )
        {
            char id_str[32];
            char p_key[32];
            UChar u_key[32];
            snprintf( id_str, 32, "%d", *id );
            dom_add_attribute( p_element, dom_attribute_create("id",id_str) );
            snprintf( p_key, 32, "%lx",(long)p );
            ascii_to_uchar( p_key, u_key, 32 );
            hashmap_put( parents, u_key, id_str );
            (*id)++;
            // check if any orphans belong to this parent
            link_node *ln = hashmap_get( orphans, u_key );
            if ( ln != NULL )
            {
                char *id_str = hashmap_get( parents, u_key );
                dom_item *child = link_node_obj( ln );
                while ( child != NULL )
                {
                    dom_add_attribute( child, dom_attribute_create("parent", 
                        id_str) );
                    ln = link_node_next( ln );
                    dom_item *child = link_node_obj( ln );
                }
                hashmap_remove( orphans, u_key, NULL );
                link_node_dispose( ln );
            }
        }
        else if ( pair_is_child(p) )
        {
            UChar u_key[32];
            char p_key[32];
            snprintf( p_key, 32, "%lx",(long)pair_parent(p) );
            ascii_to_uchar( p_key, u_key, 32 );
            char *id_str = hashmap_get( parents, u_key );
            if ( id_str != NULL )
                dom_add_attribute( p_element, dom_attribute_create("parent", 
                    id_str) );
            else
            {
                link_node *children = hashmap_get( orphans, u_key );
                if ( children == NULL )
                {
                    children = link_node_create();
                    if ( children != NULL )
                    {
                        link_node_set_obj( children, p_element );
                        res = hashmap_put( orphans, u_key, children );
                    }
                }
                else
                {
                    link_node *ln = link_node_create();
                    if ( ln != NULL )
                    {
                        link_node_set_obj( ln, p_element );
                        link_node_append( children, ln );
                    }
                    else
                        res = 0;
                }
            }
        }
        if ( res )
        {
            if ( pair_is_hint(p) )
                dom_add_attribute( p_element, 
                    dom_attribute_create("hint","true") );
            int bytes = bitset_allocated(pair_versions(p))*8;
            char *bit_str = calloc( bytes+1, 1 );
            if ( bit_str != NULL )
            {
                bitset_tostring( pair_versions(p), bit_str, bytes );
                res = dom_add_attribute( p_element, 
                    dom_attribute_create("versions",bit_str) );
                free( bit_str );
            }
            if ( res && !pair_is_child(p) )
            {
                char *utf8_data = to_utf8( pair_data(p), pair_len(p) );
                if ( utf8_data != NULL )
                {
                    res = dom_add_text( p_element, utf8_data );
                    free( utf8_data );
                }
            }
            dom_add_child( p_parent, p_element );
        }
    }
    else
        res = 0;
    return res;
}
/**
 * Write out the pairs
 * @param root the document root
 * @param mvd the mvd we are writing to
 * @param encoding the XML encoding
 * @return 1 if it worked else 0
 */
static int write_pairs( dom_item *root, MVD *mvd, char *encoding )
{
    int res = 1;
    dom_item *p_parent = dom_array_create( "pairs" );
    if ( p_parent != NULL )
    {
        hashmap *parents = hashmap_create( 12, 0 );
        hashmap *orphans = hashmap_create( 12, 0 );
        if ( parents != NULL && orphans != NULL )
        {
            int i,id = 1;
            dyn_array *pairs = mvd_get_pairs( mvd );
            for ( i=0;i<dyn_array_size(pairs);i++ )
            {
                pair *p = dyn_array_get( pairs, i );
                res = write_one_pair( p, p_parent, parents, orphans, &id );
                if ( !res )
                    break;
            }
            hashmap_dispose( parents, NULL );
            hashmap_dispose( orphans, NULL );
        }   
    }
    // attach pairs node to its parent node
    if ( res )
        res = dom_add_child( root, p_parent );
    return res;
}
/**
 * Write an MVD out as text. If there are angle brackets or 
 * ampersands in the text, these are automatically escaped 
 * as entities
 * @param mvd the source multi-version document
 * @param dst the destination MVD JSON file
 * @param encoding the desired encoding of the output 
 * (not that of the source mvd)
 * @param srcEncoding the encoding of the data in the MVD
 * @param pretty if true make some attempt at tidying the output
 * @throws an exception if it couldn't write the file or find a 
 * serialiser
 */
int mvd_json_externalise( MVD *mvd, char *dst, char *encoding ) 
{
    int res = 1;
    dom_item *root = dom_object_create( "mvd" );
    if ( root != NULL )
    {
        char *desc = to_utf8( mvd_description(mvd), 
            mvd_get_description_len(mvd) );
        if ( desc != NULL )
        {
            dom_attribute *attr = dom_attribute_create( 
                "description", desc );
            if ( attr != NULL )
            {
                res = dom_add_attribute( root, attr );
                if ( res )
                    res = dom_add_attribute( root, dom_attribute_create(
                        "encoding", mvd_get_encoding(mvd)) );
                if ( res )
                    res = write_versions( root, mvd );
                if ( res )
                    res = write_pairs( root, mvd, mvd_get_encoding(mvd) );
                if ( res )
                {
                    FILE *DST = fopen( dst, "w" );
                    if ( DST != NULL )
                    {
                        res = dom_externalise( root, DST );
                        dom_item_dispose( root );
                        fclose( DST );
                    }
                    else
                        res = 0;
                }
            }
            else
                res = 0;
        }
        else
            res = 0;
    }
    else
        res = 0;
    return res;
}
