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
#include "match.h"
#include "matcher.h"
// number of mismatched characters between consequtive matches
#define MATCH_K 2
#define PQUEUE_LIMIT 50
/**
 * A matcher is something that looks for matches by comparing the 
 * list of pairs with the suffix tree, which itself represents
 * the new version
 */
struct matcher_struct
{
    suffixtree *st;
    pair **pairs;
    int start;
    int end;
    int transpose;
    UChar *text;
    aatree *pq;
    plugin_log *log;
};

/**
 * Create a matcher
 * @param st the suffix tree already computed for the new version
 * @param text the text of the new version
 * @param pairs the pairs array from the MVD
 * @param start the index of the first pair to be aligned
 * @param end the index of the last pair to be aligned
 * @param transpose if 1 the alignment is a transposition
 * @param log record messages here
 * @return a matcher object ready to go
 */
matcher *matcher_create( suffixtree *st, UChar *text, pair **pairs, int start, 
    int end, int transpose, plugin_log *log )
{
    matcher *m = calloc( 1, sizeof(matcher) );
    if ( m != NULL )
    {
        m->log = log;
        m->start = start;
        m->end = end;
        m->pairs = pairs;
        m->text = text;
        m->st = st;
        m->pq = aatree_create( match_compare, PQUEUE_LIMIT );
        m->transpose = transpose;
        if ( m->pq == NULL )
        {
            plugin_log_add(log,"matcher: failed to create priority queue\n");
            matcher_dispose( m );
            m = NULL;
        }
    }
    else
        plugin_log_add(log, "matcher: failed to allocate object\n");
    return m;
}
/**
 * Dispose of a matcher object
 * @param m the matcher in question
 */
void matcher_dispose( matcher *m )
{
    if ( m->pq != NULL )
        aatree_dispose( m->pq );
    free( m );
}
/**
 * Extend a match as far as you can
 * @param mt the match to extend
 * @param text the UTF-16 text of the new version
 * @param log the log to record errors in
 * @return the extended/unextended match
 */
static match *match_extend( match *mt, UChar *text, plugin_log *log )
{
    match *last = mt;
    match *first = mt;
    do
    {
        match *mt2 = match_clone( mt, log );
        int distance = 1;
        do  
        {
            if ( match_single(mt2,text) && match_follows(last,mt2) )
            {// success
                match_append(last,mt2);
                mt = last = mt2;
                break;
            }
            else 
            {// failure: reset and move to next position
                if ( distance < MATCH_K )
                {
                    match_restart( mt2 );
                    distance++;
                }
                else    // give up
                {
                    match_dispose( mt2 );
                    last = NULL;
                    break;
                }
            }
        }
        while ( distance <= MATCH_K );
    } while ( last == mt );
    return first;
}
/**
 * Perform an alignment recursively
 * @param m the matcher to do the alignment within
 * @return 1 if it worked, else 0 on error
 */
int matcher_align( matcher *m )
{
    int i;
    for ( i=m->start;i<=m->end;i++ )
    {
        pair *p = m->pairs[i];
        int j,length = pair_len( p );
        for ( j=0;j<length;j++ )
        {
            match *mt = match_create( i, j, m->pairs, m->st, m->end, m->log );
            if ( mt != NULL )
            {
                match_set_versions(mt,bitset_clone(pair_versions(m->pairs[i])));
                if ( match_single(mt,m->text) )
                {
                    match *queued = match_extend( mt, m->text, m->log );
                    match *existing = aatree_add( m->pq, queued );
                    if ( existing != NULL )
                    {
                        match_print( queued, m->text );
                        match_inc_freq( existing );
                        if ( existing == queued )
                            continue;
                    }
                    match_dispose( queued );
                }
                else
                    match_dispose( mt );
            }
            else
                break;
        }
    }
    return !aatree_empty(m->pq);
}
/**
 * Get the longest maximal unique match (MUM)
 * @param m the matcher in question
 * @return a match, which may be a chain of matches
 */
match *matcher_get_mum( matcher *m )
{
    match *found = NULL;
    while ( !aatree_empty(m->pq) )
    {
        match *mt = aatree_max( m->pq );
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