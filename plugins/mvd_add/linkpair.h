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
void linkpair_left( linkpair *lp );
void linkpair_right( linkpair *lp );
void linkpair_set_st_off( linkpair *lp, int st_off );
void linkpair_st_off( linkpair *lp );

#ifdef	__cplusplus
}
#endif

#endif	/* LINKPAIR_H */

