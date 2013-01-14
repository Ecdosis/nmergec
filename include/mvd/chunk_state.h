/* 
 * File:   chunk_state.h
 * Author: desmond
 *
 * Created on January 12, 2013, 6:02 AM
 */

#ifndef CHUNK_STATE_H
#define	CHUNK_STATE_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * The state of Chunks
 * @author Desmond Schmidt 11/11/07 revised 21/5/09
 */
typedef enum 
{
	/** default state as e.g. background text after find */
	a_chunk,
	/** text of second version after compare */
	added,
	/** background version for partial version */
	backup,
	/** child of transposition */
	child,
    /** text of first version after compare */
	deleted,
	/** text found by search */
	found,
	/** merged state after compare for shared text */
	merged,
	/** parent of transposition */
	parent,
	/** attested partial version text */
	partial
} chunk_state;

chunk_state chunk_state_value( const char *value );
#ifdef MVD_TEST
int test_chunk_state( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* CHUNK_STATE_H */

