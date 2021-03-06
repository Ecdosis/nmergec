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
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "card.h"
#include "location.h"
#include "match_state.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif
/**
 * Store the state of a match while matching. This is done whenever there 
 * is more than one match possible at any point in the pairs list, 
 * corresponding to a branch in the variant graph
 */
struct match_state_struct
{
    location start;
    location end;
    location prev;
    int text_off;
    int maximal;
    int len;
    bitset *bs;
    bitset *prev_bs;
    pos *loc;
    match_state *next;
};

/**
 * Create a match state at a branch point in the variant graph
 * @param start the start location for the match to resume from
 * @param end the end location that has already been matched
 * @param prev the last match position
 * @param st_off the offset in the suffix tree if set
 * @param bs the versions of this state (already created)
 * @param prev_bs the previous versions of the match
 * @param loc the position in the suffix tree to which we have matched
 * @param maximal 1 if this is maximal
 * @return a match state object
 */
match_state *match_state_create( location *start, location *end, location *prev,
    int text_off, int len, bitset *bs, bitset *prev_bs, pos *loc, int maximal, 
    plugin_log *log )
{
    match_state *ms = calloc( 1, sizeof(match_state) );
    if ( ms != NULL )
    {
        ms->start = *start;
        ms->end = *end;
        ms->prev = *prev;
        ms->text_off = text_off;
        ms->len = len;
        if ( bs != NULL )
            ms->bs = bitset_clone( bs );
        if ( prev_bs != NULL )
            ms->prev_bs = bitset_clone( prev_bs );
        ms->maximal = maximal;
        ms->loc = calloc( 1, sizeof(pos) );
        if ( ms->loc == NULL )
        {
            plugin_log_add(log,"match_state: failed to allocate position\n");
            match_state_dispose( ms );
            ms = NULL;
        }
        else
        {
            ms->loc->loc = loc->loc;
            ms->loc->v = loc->v;
        }
    }
    else
        plugin_log_add( log, "match_state: failed to create object\n");
    return ms;
}
/**
 * Copy a list of match states
 * @param ms the head of the list
 * @param log the log to record errors in
 */
match_state *match_state_copy( match_state *ms, plugin_log *log )
{
    match_state *ms_copy = match_state_create( &ms->start, &ms->end, &ms->prev,
        ms->text_off, ms->len, ms->bs, ms->prev_bs, ms->loc, ms->maximal, log );
    if ( ms_copy != NULL )
    {
        if ( ms->next != NULL )
            ms_copy->next = match_state_copy( ms->next, log );
    }
    return ms_copy;
}
/**
 * Dispose of a match state object
 * @param ms the match state to dispose of
 */
void match_state_dispose( match_state *ms )
{
    if ( ms->bs != NULL )
        bitset_dispose( ms->bs );
    if ( ms->prev_bs != NULL )
        bitset_dispose( ms->prev_bs );
    if ( ms->loc != NULL )
        free( ms->loc );
    free( ms );
}
/**
 * Add a match state to the queue
 * @param head the head of the queue
 * @param ms the new tail
 */
void match_state_push( match_state *head, match_state *ms )
{
    while ( head->next != NULL )
        head = head->next;
    head->next = ms;
}
/**
 * Get the next match state
 * @param ms the match state in question
 * @return the match state object
 */
match_state *match_state_next( match_state *ms )
{
    return ms->next;
}
// accessors used for restoring state to the match
void match_state_loc( match_state *ms, pos *loc )
{
    *loc = *(ms->loc);
}
location match_state_start( match_state *ms )
{
    return ms->start;
}
location match_state_end( match_state *ms )
{
    return ms->end;
}
location match_state_prev( match_state *ms )
{
    return ms->prev;
}
bitset *match_state_prev_bs( match_state *ms )
{
    return ms->prev_bs;
}
int match_state_text_off( match_state *ms )
{
    return ms->text_off;
}
int match_state_len( match_state *ms )
{
    return ms->len;
}
bitset *match_state_bs( match_state *ms )
{
    return ms->bs;
}
int match_state_maximal( match_state *ms )
{
    return ms->maximal;
}
