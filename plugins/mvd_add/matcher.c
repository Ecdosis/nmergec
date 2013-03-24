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
#include "hashmap.h"
#include "linkpair.h"
#include "match.h"
#include "matcher.h"
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
    aatree *pq;
    hashmap *avoids;
    plugin_log *log;
};
/**
 * Create a matcher
 * @param st the suffix tree already computed for the new version
 * @param pq the priority queue to store the best MUMs
 * @param text the text of the new version
 * @param pairs the list of pairs from the MVD
 * @param log record messages here
 * @return a matcher object ready to go
 */
matcher *matcher_create( suffixtree *st, aatree *pq, UChar *text, 
    linkpair *pairs, plugin_log *log )
{
    matcher *m = calloc( 1, sizeof(matcher) );
    if ( m != NULL )
    {
        m->log = log;
        m->pairs = pairs;
        m->text = text;
        m->st = st;
        m->pq = pq;
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
        hashmap_dispose( m->avoids, free );
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
        int j,length = pair_len( p );
        for ( j=0;j<length;j++ )
        {
            UChar key[16];
            calc_ukey( key, (long)lp, 16 );
            if ( m->avoids!=NULL && hashmap_contains(m->avoids,key) )
                hashmap_remove( m->avoids,key,free );
            else
            {
                match *mt = match_create( lp, j, m->pairs, m->st, m->log );
                if ( mt != NULL )
                {
                    match_set_versions( mt, bitset_clone(pair_versions(p)));
                    if ( match_single(mt,m->text) )
                    {
                        match *queued = match_extend( mt, m->avoids, m->text, 
                            m->log );
                        match *existing = aatree_add( m->pq, queued );
                        if ( existing != NULL )
                        {
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
        lp = linkpair_right( lp );
    }
    return !aatree_empty(m->pq);
}