#include <stdlib.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "pair.h"
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "matcher.h"
#include "state.h"
#include "aatree.h"
#include "match.h"
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
    aatree *pq;
    plugin_log *log;
};

/**
 * Create a matcher
 * @param st the suffix tree already computed for the new version
 * @param pairs the pairs array from the MVD
 * @param start the index of the first pair to be aligned
 * @param end the index of the last pair to be aligned
 * @param transpose if 1 the alignment is a transposition
 * @param log record messages here
 * @return a matcher object ready to go
 */
matcher *matcher_create( suffixtree *st, pair **pairs, int start, 
    int end, int transpose, plugin_log *log )
{
    matcher *m = calloc( 1, sizeof(matcher) );
    if ( m != NULL )
    {
        m->log = log;
        m->start = start;
        m->end = end;
        m->pairs = pairs;
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
 * Perform an alignment recursively
 * @param m the matcher to do the alignment within
 * @return 1 if it worked, else 0 on error
 */
int matcher_align( matcher *m )
{
    int i;
    match *queued = NULL;
    for ( i=m->start;i<=m->end;i++ )
    {
        pair *p = m->pairs[i];
        int j,length = pair_len( p );
        for ( j=0;j<length;j++ )
        {
            match *mt = match_create( i, j, m->pairs, m->end, m->log );
            if ( mt != NULL )
            {
                match_set_versions(mt,bitset_clone(pair_versions(m->pairs[i])));
                if ( match_single(mt) )
                {
                    match *last = queued = mt;
                    do
                    {
                        match *mt2 = match_clone( mt, m->log );
                        match_single( mt2 );
                        if ( match_follows(last,mt2) )
                        {
                            match_append(last,mt2);
                            last = mt2;
                        }
                        else
                            break;
                    } while ( 1 );
                    if ( !aatree_add(m->pq,queued) )
                        match_dispose( queued );
                }
                else
                    match_dispose( mt );
            }
            else
                break;
        }
    }
}