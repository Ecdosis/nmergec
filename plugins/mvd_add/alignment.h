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
alignment *alignment_create( UChar *text, int start, int len, int tlen, 
    int version, orphanage *o, plugin_log *log );
void alignment_print( alignment *a, const char *prompt );
void alignment_dispose( alignment *a );
UChar *alignment_text( alignment *a, int *tlen );
alignment *alignment_push( alignment *head, alignment *tail );
alignment *alignment_pop( alignment *head );
int alignment_version( alignment *a );
plugin_log *alignment_log( alignment *a );
suffixtree *alignment_suffixtree( alignment *a );
int alignment_len( alignment *a );
int alignment_tlen( alignment *a );
int alignment_align( alignment *a, linkpair **pairs, 
    alignment **left, alignment **right, orphanage *o );
int alignment_start( alignment *a );
#ifdef	__cplusplus
}
#endif

#endif	/* ALIGNMENT_H */
