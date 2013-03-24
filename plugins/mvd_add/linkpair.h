/* 
 * File:   linkpair.h
 * Author: desmond
 *
 * Created on March 21, 2013, 6:20 AM
 */

#ifndef LINKPAIR_H
#define	LINKPAIR_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct linkpair_struct linkpair;
linkpair *linkpair_create( pair *p, plugin_log *log );
void linkpair_dispose( linkpair *lp );
void linkpair_set_left( linkpair *lp, linkpair *left );
void linkpair_set_right( linkpair *lp, linkpair *right );
linkpair *linkpair_left( linkpair *lp );
linkpair *linkpair_right( linkpair *lp );
linkpair *linkpair_next( linkpair *lp, bitset *bs );
void linkpair_set_st_off( linkpair *lp, int st_off );
int linkpair_st_off( linkpair *lp );
pair *linkpair_pair( linkpair *lp );

#ifdef	__cplusplus
}
#endif

#endif	/* LINKPAIR_H */

