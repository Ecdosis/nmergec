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
#include "linkpair.h"
#include "match.h"
#include "alignment.h"
#include "matcher.h"
#include "hashmap.h"
#include "utils.h"

/**
 * A matcher is something that looks for matches by comparing the 
 * list of pairs with the suffix tree, which itself represents
 * the new version. After matching is ended matcher an return the 
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
        if ( bitset_next_set_bit(bs,m->version)!=m->version )
        {
            int j,length = pair_len( p );
            for ( j=0;j<length;j++ )
            {
                match *mt = match_create( lp, j, m->pairs, m->st, m->log );
                if ( mt != NULL )
                {
                    match_set_versions( mt, bitset_clone(pair_versions(p)));
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