#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dom.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define BLOCK_SIZE 4
#define INDENT 4
static char *value_fmt1 = "\"%s\": \"%s\",\n";
static char *value_fmt2 = "\"%s\": %s,\n";
    
/**
 * A basic JSON export. 
 */
struct attribute_struct
{
    char *key;
    char *value;
};
struct element_struct
{
    char *name;
    dom_attribute **attrs;
    int allocated;
    int used;
    char *text;
    dom_element *children;
    dom_element *next;
};
/**
 * Create a dom attribute
 * @param key the key
 * @param value the attribute value
 * @return an finished attribute
 */
dom_attribute *dom_attribute_create( char *key, char *value )
{
    dom_attribute *a = calloc( 1, sizeof(dom_attribute) );
    if ( a != NULL )
    {
        a->key = strdup(key);
        a->value = strdup( value );
        if ( a->key==NULL || a->value==NULL )
        {
            dom_attribute_dispose( a );
            a = NULL;
        }
    }
    return a;
}
/**
 * Dispose of an attribute
 * @param a the attribute to destroy
 */
void dom_attribute_dispose( dom_attribute *a )
{
    if ( a->key != NULL )
        free( a->key );
    if ( a->value != NULL )
        free( a->value );
    free( a );
}
/**
 * Create an element
 * @param name the name of the element
 * @return a finsihed dom element or NULL
 */
dom_element *dom_element_create( char *name )
{
    dom_element *e = calloc( 1, sizeof(dom_element) );
    if ( e != NULL )
    {
        e->name = strdup( name );
        if ( e->name == NULL )
        {
            dom_element_dispose( e );
            e = NULL;
        }
    }
    return e;
}
/**
 * Dispose of a dom element and its attributes
 * @param de the dom element in question
 */
void dom_element_dispose( dom_element *de )
{
    if ( de->name != NULL )
        free( de->name );
    int i = 0;
    while ( de->attrs[i] != NULL )
        dom_attribute_dispose( de->attrs[i++] );
    if ( de->children != NULL )
    {
        i=0;
        dom_element *child = de->children;
        while ( child != NULL )
        {
            dom_element_dispose( child );
            child = child->next;
        }
    }
    if ( de->text != NULL )
        free( de->text );
    free( de );
}
/**
 * Add an attribute to the element
 * @param de the dom element in question
 * @param attr the attribute
 * @return 1 if it worked else 0
 */
int dom_add_attribute( dom_element *de, dom_attribute *attr )
{
    int res = 1;
    // to simply things we handle this error here
    if ( attr != NULL )
    {
        if ( de->used+1 >= de->allocated )
        {
            dom_attribute **attrs = calloc( de->allocated+BLOCK_SIZE, 
                sizeof(dom_attribute*) );
            if ( attrs != NULL )
            {
                int i = 0;
                if ( de->attrs != NULL )
                {
                    while ( de->attrs[i] != NULL )
                        attrs[i] = de->attrs[i++];
                }
                if ( de->attrs != NULL )
                    free( de->attrs );
                de->attrs = attrs;
                de->allocated += BLOCK_SIZE;
            }
            else
                res = 0;
        }
        de->attrs[de->used++] = attr;
    }
    else
        res = 0;
    return res;
}
/**
 * Write a number of spaces
 * @param dst the file to write to
 * @param spaces the number of spaces
 * @return 1 if it worked else 0
 */
static int dom_write_spaces( FILE *dst, int spaces )
{
    int i;
    for ( i=0;i<spaces;i++ )
    {
        size_t n = fwrite(" ", 1, 1, dst );
        if ( n != 1 )
            return 0;
    }
    return 1;
}
/**
 * Is a value boolean?
 * @param value the value to test
 * @return 1 if it is either "true" or "false" else 0
 */
static int dom_is_boolean( char *value )
{
    return strcmp(value,"true")==0||strcmp(value,"false")==0;
}
/**
 * Is a value an integer?
 * @param value the value to test
 * @return 1 if it is a pure integer
 */
static int dom_is_integer( char *value )
{
    int i,len = strlen( value );
    for ( i=0;i<len;i++ )
        if ( !isdigit(value[i]) )
            return 0;
    return 1;
}
/**
 * Write an attribute to disk
 * @param attr the attribute to write out
 * @param dst the destination file
 * @return 1 if it worked else 0
 */
static int dom_write_attribute( dom_attribute *attr, int depth, FILE *dst )
{
    int n,res = dom_write_spaces( dst, depth );
    if ( res )
    {
        if ( dom_is_boolean(attr->value) || dom_is_integer(attr->value) )
        {
            int n = fprintf(dst, value_fmt2,attr->key,attr->value);
            res = (n==6+strlen(attr->key)+strlen(attr->value));
        }
        else
        {
            int n = fprintf(dst, value_fmt1,attr->key,attr->value);
            res = (n==8+strlen(attr->key)+strlen(attr->value));
        }
    }
    return res;
}
/**
 * Write a start tag to disk
 * @param de the dom element in question
 * @param dst the file to write to
 * @param depth the number of spaces to indent
 * @param log the log to record errors in
 * @return 1 if it worked else 0 
 */
static int dom_element_start( dom_element *de, FILE *dst, int depth )
{
    int n,res = dom_write_spaces( dst, depth );
    if ( res )
    {
        // start-tag
        n = fprintf( dst, "\"%s\": {\n",de->name );
        if ( n != 6+strlen(de->name) )
            res = 0;
        int i = 0;
        if ( de->attrs != NULL )
        {
            while ( res && de->attrs[i] != NULL )
                res = dom_write_attribute( de->attrs[i++], depth+INDENT, dst );
        }
    }
    return res;
}
/**
 * Write the end tag
 * @param de the element
 * @param dst the destination file
 * @param add a comma at the end
 * @return 1 if it worked else 0
 */
static int dom_element_end( dom_element *de, FILE *dst, int depth, int comma )
{
    int n,res=1;
    res = dom_write_spaces( dst, depth );
    if ( res )
    {
        if ( !comma )
        {
            n = fprintf( dst, "}\n" );
            if ( n != 2 )
                res = 0;
        }
        else 
        {
            n = fprintf( dst, "},\n" );
            if ( n != 3 )
                res = 0;
        }
    }
    return res;
}
char *resize( char *dst, int pos, int slen, int *limit )
{
    int new_limit = *limit+slen+100;
    char *new_dst = calloc( new_limit, 1 );
    if ( new_dst != NULL )
    {
        memcpy( new_dst, dst, pos );
        free( dst );
        dst = new_dst;
        *limit = new_limit;
    }
    else
        return NULL;
    return dst;
}
/**
 * Concatenate onto a dynamic string
 * @param dst the dynamic destination
 * @param src the source string
 * @param pos the updatable position in dst
 * @param limit the updatable limit to dst's length
 * @return the reallocated or original string
 */
static char *concat( char *dst, char *src, int *pos, int *limit )
{
    int slen = strlen(src);
    if ( *pos+slen >= *limit )
    {
        dst = resize( dst, slen, *pos, limit );
        if ( dst == NULL )
            return dst;
    }
    memcpy( &dst[*pos], src, slen );
    *pos += slen;
    return dst;
}
/**
 * Replace all double-quotation marks with escaped versions
 * @param text the raw text
 * @return an allocated string containing the escaped text
 */
static char *dom_escape( char *text )
{
    int tlen = strlen(text);
    int limit = tlen*10/9;
    char *dst = calloc( limit, 1 );
    int pos = 0;
    if ( dst != NULL )
    {
        int i;
        for ( i=0;i<tlen;i++ )
        {
            switch ( text[i] )
            {
                case '"':
                    dst = concat( dst, "\\\"", &pos, &limit );
                    break;
                case '\\':
                    dst = concat( dst, "\\\\", &pos, &limit );
                    break;
                case '/':
                    dst = concat( dst, "\\/", &pos, &limit );
                    break;
                case '\b':
                    dst = concat( dst, "\\b", &pos, &limit );
                    break;
                case '\f':
                    dst = concat( dst, "\\f", &pos, &limit );
                    break;
                case '\n':
                    dst = concat( dst, "\\n", &pos, &limit );
                    break;
                case '\r':
                    dst = concat( dst, "\\r", &pos, &limit );
                    break;
                case '\t':
                    dst = concat( dst, "\\t", &pos, &limit );
                    break;
                default:
                    if ( pos+1 >= limit )
                    {
                        dst = resize( dst, 1, pos, &limit );
                        if ( dst == NULL )
                            break;
                    }
                    dst[pos++] = text[i];
                    break;
            }
        }
    }
    return dst;
}
/**
 * Write out an entire element and its children
 * @param de the element to write out
 * @param indent the number of space to indent the element
 * @param dst the file to write to
 * @return 1 if it worked else 0
 */
static int dom_write_element( dom_element *de, int indent, FILE *dst )
{
    int n,res = dom_element_start( de, dst, indent );
    if ( res )
    {
        if ( de->children != NULL )
        {
            dom_element *child = de->children;
            while ( res && child != NULL )
            {
                res = dom_write_element( child, indent+INDENT, dst );
                child = child->next;
            }
        }
        else if ( de->text != NULL )
        {
            char *escaped = dom_escape(de->text);
            if ( escaped != NULL )
            {
                res = dom_write_spaces(dst,indent+4);
                if ( res )
                {
                    n = fprintf(dst,"text: \"%s\"\n",escaped );
                    if ( n != strlen(escaped)+9 )
                        res = 0;
                }
                free( escaped );
            }
        }
        if ( res )
        {
            res = dom_element_end( de, dst, indent, de->next!=NULL );
        }
    }
    return res;
}
/**
 * Write a dom out to disk
 * @param root the root element
 * @param dst the destination file
 * @return 1 if it worked else 0
 */
int dom_externalise( dom_element *root, FILE *dst )
{
    return dom_write_element( root, 0, dst );
}
/**
 * Add a child to this element. Must NOT have any text.
 * @param parent the parent to get the new child
 * @param child the child to add
 * @return 1 if all was OK else 0
 */
int dom_add_child( dom_element *parent, dom_element *child )
{
    int res = 1;
    if ( parent->text != NULL )
        res = 0;
    else if ( parent->children == NULL )
    {
        parent->children = child;
    }
    else
    {
        dom_element *sibling = parent->children;
        while ( sibling->next != NULL )
            sibling = sibling->next;
        sibling->next = child;
    }
    return res;
}
/**
 * Add some text to a node. Must NOT have any children.
 * @param de
 * @param text the text to add
 * @return 1 if it worked else 0
 */
int dom_add_text( dom_element *de, char *text )
{
    int res = 1;
    if ( de->children == NULL )
        de->text = strdup(text);
    if ( de->text == NULL )
        res = 0;
    return res;
}
#ifdef DOM_TEST
int main( int argc, char **argv )
{
    dom_element *root = dom_element_create( "root" );
    dom_add_attribute( root, dom_attribute_create("encoding","utf-8") );
    dom_element *child = dom_element_create("child");
    dom_add_child( root, child );
    dom_add_attribute( child, dom_attribute_create("bananas","4"));
    dom_add_attribute( child, dom_attribute_create("pears","2"));
    dom_element *grandchild1 = dom_element_create("grandchild" );
    dom_element *grandchild2 = dom_element_create("grandchild" );
    dom_add_text( grandchild1, "The quick brown fox");
    dom_add_text( grandchild2, "jumps over the lazy dog");
    dom_add_child( child, grandchild1 );
    dom_add_child( child, grandchild2 );
    dom_add_attribute( grandchild1, dom_attribute_create("n","27") );
    dom_add_attribute( grandchild2, dom_attribute_create("validity","true") );
    FILE *dst = fopen("test.json","w");
    if ( dst != NULL )
    {
        int res = dom_externalise( root,dst );
        fclose( dst );
        if ( !res )
            fprintf(stderr,"dom: failed to write test\n");
    }
}
#endif