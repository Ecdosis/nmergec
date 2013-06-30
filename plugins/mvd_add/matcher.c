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
#include <stdlib.h>
#include <stdio.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "state.h"
#include "aatree.h"
#include "dyn_array.h"
#include "linkpair.h"
#include "match.h"
#include "alignment.h"
#include "matcher.h"
#include "hashmap.h"
#include "utils.h"

/**
 * A matcher is something that looks for matches by comparing the 
 * list of pairs with the suffix tree, which itself represents
 * the new version. After matching is ended matcher returns the 
 * best MUM.
 */
struct matcher_struct
{
    suffixtree *st;
    linkpair *pairs;
    UChar *text;
    int tlen;
    aatree *pq;
    int version;
    plugin_log *log;
};
/**
 * Create a matcher
 * @param a the alignment object
 * @param pq the priority queue to store the best MUMs
 * @param pairs the list of pairs from the MVD
 * @return a matcher object ready to go
 */
matcher *matcher_create( alignment *a, aatree *pq, linkpair *pairs )
{
    matcher *m = calloc( 1, sizeof(matcher) );
    if ( m != NULL )
    {
        m->log = alignment_log( a );
        m->pairs = pairs;
        m->text = alignment_text( a, &m->tlen );
        m->version = alignment_version(a);
        m->st = alignment_suffixtree( a );
        m->pq = pq;
    }
    else
        plugin_log_add(m->log, "matcher: failed to allocate object\n");
    return m;
}
/**
 * Dispose of a matcher object
 * @param m the matcher in question
 */
void matcher_dispose( matcher *m )
{
    free( m );
}
/**
 * Perform an alignment recursively
 * @param m the matcher to do the alignment within
 * @return 1 if it worked, else 0 on error
 */
int matcher_align( matcher *m )
{
    linkpair *lp = m->pairs;
    while ( lp != NULL )
    {
        pair *p = linkpair_pair( lp );
        bitset *bs = pair_versions(p);
        // ignore pairs already aligned with the new version
        if ( bitset_next_set_bit(bs,m->version)!=m->version )
        {
            int j,length = pair_len( p );
            for ( j=0;j<length;j++ )
            {
                // start a match at each character position
                match *mt = match_create( lp, j, m->pairs, m->st, m->log );
                if ( mt != NULL )
                {
                    match_set_versions( mt, bitset_clone(bs) );
                    while ( mt != NULL )
                    {
                        match *queued = NULL;
                        match *existing = NULL;
                        if ( match_single(mt,m->text,m->log) )
                        {
                            queued = match_extend( mt, m->text, m->log );
                            existing = aatree_add( m->pq, queued );
                            if ( existing != NULL )
                                match_inc_freq( existing );
                            // if we added it to the queue, freeze its state
                            if ( existing == queued )
                                mt = match_copy( mt, m->log );
                        }    
                        if ( mt != NULL && !match_pop(mt) )
                        {
                            match_dispose( mt );
                            mt = NULL;
                        }
                    }
                }
                else
                    break;
            }
        }
        lp = linkpair_right( lp );
    }
    return !aatree_empty(m->pq);
}
/**
 * Get the longest maximal *unique* match (MUM)
 * @param m the matcher in question
 * @return a match, which may be a chain of matches
 */
match *matcher_get_mum( matcher *m )
{
    match *found = NULL;
    while ( !aatree_empty(m->pq) )
    {
        match *mt = aatree_max( m->pq );
        //match *mt = aatree_min( m->pq );
        if ( match_freq(mt) == 1 )
        {
            found = mt;
            break;
        }
        else if ( !aatree_delete(m->pq,mt) )
            break;
    }
    return found;
}