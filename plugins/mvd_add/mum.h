/* 
 * File:   mum.h
 * Author: desmond
 *
 * Created on 25 April 2014, 3:31 PM
 */

#ifndef MUM_H
#define	MUM_H

#ifdef	__cplusplus
extern "C" {
#endif
    
typedef struct mum_struct mum;
mum *mum_create( match *mt, int v, plugin_log *log );
void mum_dispose( mum *m );
int mum_len( mum *m );
int mum_total_len( mum *m );
card *mum_start_card( mum *m );
card *mum_end_card( mum *m );
bitset *mum_versions( mum *m );
int mum_text_off( mum *m );
mum *mum_next( mum *m );
int mum_text_end( mum *m );
int mum_split( mum *m, UChar *text, int v, orphanage *o, 
    dyn_array *discards, plugin_log *log );
int mum_transposed( mum *m, int new_version, int tlen, int *dist );
int mum_verify( mum *m, int v );

#ifdef	__cplusplus
}
#endif

#endif	/* MUM_H */

