/* 
 * File:   alignment.h
 * Author: desmond
 *
 * Created on 15 March 2013, 3:32 PM
 */

#ifndef ALIGNMENT_H
#define	ALIGNMENT_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct alignment_struct alignment;
alignment *alignment_create( UChar *text, int tlen, int start_p, int end_p, 
    plugin_log *log );
void alignment_dispose( alignment *a );
UChar *alignment_text( alignment *a, int *tlen );
void alignment_push( alignment *head, alignment *tail );
alignment *alignment_pop( alignment *head );
int alignment_align( alignment *head, pair **pairs, 
    alignment **left, alignment **right, plugin_log *log );
#ifdef	__cplusplus
}
#endif

#endif	/* ALIGNMENT_H */
