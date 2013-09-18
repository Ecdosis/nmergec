/*
 * Copyright 2013 Desmond Schmidt
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* 
 * File:   tree.h
 * Author: desmond
 *
 * Created on February 4, 2013, 10:01 AM
 */

#ifndef NODE_H
#define	NODE_H

#ifdef	__cplusplus
extern "C" {
#endif


typedef struct node_struct node;
typedef struct node_iterator_struct node_iterator;
node *node_create( int start, int len, plugin_log *log );
node *node_create_leaf( int i, plugin_log *log );
void node_dispose( node *v );
int node_add_leaf( node *parent, int i, UChar *str, plugin_log *log );
int node_num_children( node *v );
node_iterator *node_children( node *parent, plugin_log *log );
int node_iterator_has_next( node_iterator *iter );
node *node_iterator_next( node_iterator *iter );
void node_iterator_dispose( node_iterator *iter );
void node_add_child( node *parent, node *child, UChar *str, plugin_log *log );
int node_is_leaf( node *v );
node *node_split( node *v, int loc, UChar *str, plugin_log *log );
void node_set_link( node *v, node *link );
void node_set_len( node *v, int len );
node *node_parent( node *v );
void node_clear_next( node *v );
int node_has_next( node *v );
int node_len( node *v );
int node_start( node *v );
node *node_link( node *v );
int node_kind( node *v );
int node_end( node *v, int max );
void node_print_children( UChar *str, node *v, plugin_log *log );
node *node_find_child( node *v, UChar *str, UChar c );
UChar node_first_char( node *v, UChar *str );
void node_test( int *passed, int *failed );

#ifdef	__cplusplus
}
#endif

#endif	/* NODE_H */

