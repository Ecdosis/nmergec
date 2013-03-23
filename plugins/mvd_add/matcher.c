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
#include "pos_item.h"
#include "match.h"
#include "matcher.h"

/**
 * A matcher is something that looks for matches by comparing the 
 * list of pairs with the suffix tree, which itself represents
 * the new version. After matching is ended matcher an return the 
 * best MUM.
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
    pos_item *avoids;
    plugin_log *log;
};

/**
 * Create a matcher
 * @param st the suffix tree already computed for the new version
 * @param pq the priority queue to store the best MUMs
 * @param text the text of the new version
 * @param pairs the pairs array from the MVD
 * @param start the index of the first pair to be aligned
 * @param end the index of the last pair to be aligned
 * @param transpose if 1 the alignment is a transposition
 * @param log record messages here
 * @return a matcher object ready to go
 */
matcher *matcher_create( suffixtree *st, aatree *pq, UChar *text, 
    pair **pairs, int start, int end, int transpose, plugin_log *log )
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
        m->pq = pq;
        m->transpose = transpose;
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
    if ( m->avoids != NULL )
        pos_item_dispose( m->avoids );
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
    for ( i=m->start;i<=m->end;i++ )
    {
        pair *p = m->pairs[i];
        int j,length = pair_len( p );
        for ( j=0;j<length;j++ )
        {
            if ( m->avoids!=NULL && pos_item_peek(m->avoids,i,j) )
                m->avoids = pos_item_pop( m->avoids );
            else
            {
                match *mt = match_create( i, j, m->pairs, m->st, m->end, 
                    m->log );
                if ( mt != NULL )
                {
                    match_set_versions(mt,
                        bitset_clone(pair_versions(m->pairs[i])));
                    if ( match_single(mt,m->text) )
                    {
                        match *queued = match_extend( mt, &m->avoids, m->text, 
                            m->log );
                        match *existing = aatree_add( m->pq, queued );
                        if ( existing != NULL )
                        {
                            //match_print( queued, m->text );
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
    }
    return !aatree_empty(m->pq);
}