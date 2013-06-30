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
typedef struct element_struct dom_element;
typedef struct attribute_struct dom_attribute;
dom_attribute *dom_attribute_create( char *key, char *value );
void dom_attribute_dispose( dom_attribute *a );
dom_element *dom_element_create( char *name );
void dom_element_dispose( dom_element *de );
int dom_add_attribute( dom_element *de, dom_attribute *attr );
int dom_add_child( dom_element *parent, dom_element *child );
int dom_add_text( dom_element *de, char *text );
int dom_externalise( dom_element *root, FILE *dst );


#ifdef	__cplusplus
}
#endif

#endif	/* DOM_H */

