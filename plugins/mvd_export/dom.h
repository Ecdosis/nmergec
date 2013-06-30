/* 
 * File:   dom.h
 * Author: desmond
 *
 * Created on June 29, 2013, 4:55 PM
 */

#ifndef DOM_H
#define	DOM_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct attribute_struct dom_attribute;
typedef struct item_struct dom_item;
dom_attribute *dom_attribute_create( char *key, char *value );
void dom_attribute_dispose( dom_attribute *a );
dom_item *dom_object_create( char *name );
dom_item *dom_array_create( char *name );
void dom_item_dispose( dom_item *di );
int dom_add_attribute( dom_item *di, dom_attribute *attr );
int dom_add_child( dom_item *parent, dom_item *child );
int dom_add_text( dom_item *de, char *text );
int dom_externalise( dom_item *root, FILE *dst );


#ifdef	__cplusplus
}
#endif

#endif	/* DOM_H */

