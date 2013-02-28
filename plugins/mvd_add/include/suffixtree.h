/* 
 * File:   suffixtree.h
 * Author: desmond
 *
 * Created on February 13, 2013, 10:22 AM
 */

#ifndef SUFFIXTREE_H
#define	SUFFIXTREE_H

#ifdef	__cplusplus
extern "C" 
{
#endif
    
typedef struct suffixtree_struct suffixtree;
suffixtree *suffixtree_create( UChar *txt, size_t tlen, plugin_log *log );
void suffixtree_dispose( suffixtree *st );

#ifdef	__cplusplus
}
#endif

#endif	/* SUFFIXTREE_H */

