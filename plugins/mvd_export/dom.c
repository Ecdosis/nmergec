#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dom.h"
#include "dyn_array.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define BLOCK_SIZE 4
#define INDENT 4
#define OBJECT_KIND 1
#define ARRAY_KIND 2
typedef struct object_struct dom_object;
typedef struct array_struct dom_array;

static char *fmt_quoted = "\"%s\": \"%s\",\n";
static char *fmt_unquoted = "\"%s\": %s,\n";
static char *fmt_quoted_final = "\"%s\": \"%s\"\n";
static char *fmt_unquoted_final = "\"%s\": %s\n";
/**
 * A basic JSON export. 
 */
struct attribute_struct
{
    char *key;
    char *value;
};
struct object_struct
{
    dom_attribute **attrs;
    int allocated;
    int used;
    char *text;
    dom_item *children;
};
struct array_struct
{
    dyn_array *items;
};
struct item_struct
{
    int kind;
    char *name;
    dom_item *next;
    union
    {
        struct object_struct elem;
        struct array_struct array;
    };
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
 * Dispose of a dom item (object or array)
 * @param di the item to dispose
 */
void dom_item_dispose( dom_item *di )
{
    if ( di->name != NULL )
        free( di->name );
    if ( di->kind == ARRAY_KIND )
    {
        if ( di->array.items != NULL )
        {
            int i,nitems = dyn_array_size( di->array.items );
            for ( i=0;i<nitems;i++ )
            {
                dom_item *di = dyn_array_get( di->array.items, i );
                dom_item_dispose( di );
            }
            dyn_array_dispose( di->array.items );
        }
    }
    else if ( di->kind == OBJECT_KIND )
    {
        int i=0;
        while ( di->elem.attrs[i] != NULL )
            dom_attribute_dispose( di->elem.attrs[i++] );
        if ( di->elem.children != NULL )
        {
            i=0;
            dom_item *child = di->elem.children;
            while ( child != NULL )
            {
                dom_item_dispose( child );
                child = child->next;
            }
        }
        if ( di->elem.text != NULL )
            free( di->elem.text );
    }
    free( di );
}
/**
 * Create a dom item (object or array)
 * @param kind the kind of item
 * @return a finished item
 */
static dom_item *dom_item_create( char *name, int kind )
{
    dom_item *di = calloc( 1, sizeof(dom_item) );
    if ( di != NULL )
    {
        di->kind = kind;
        di->name = strdup( name );
        if ( di->name == NULL )
        {
            dom_item_dispose( di );
            di = NULL;
        }
        else if ( kind == ARRAY_KIND )
        {
            di->array.items = dyn_array_create( 12 );
            if ( di->array.items == NULL )
            {
                dom_item_dispose( di );
                di = NULL;
            }   
        }
    }
    return di;
}
/**
 * Create an object
 * @param name the name of the object
 * @return a finished dom object or NULL
 */
dom_item *dom_object_create( char *name )
{
    return dom_item_create( name, OBJECT_KIND );
}
/**
 * Create a dom array
 * @return the array
 */
dom_item *dom_array_create( char *name )
{
    return dom_item_create( name, ARRAY_KIND );
}
/**
 * Get the next item
 * @param di the current item
 * @return the next one or NULL
 */
static dom_item *dom_item_next( dom_item *di )
{
    return di->next;
}
/**
 * Add an attribute to the item if it is an object
 * @param de the dom item (object) in question
 * @param attr the attribute
 * @return 1 if it worked else 0
 */
int dom_add_attribute( dom_item *di, dom_attribute *attr )
{
    int res = 1;
    // to simply things we handle this error here
    if ( di->kind == OBJECT_KIND )
    {
        if ( attr != NULL )
        {
            if ( di->elem.used+1 >= di->elem.allocated )
            {
                dom_attribute **attrs = calloc( di->elem.allocated+BLOCK_SIZE, 
                    sizeof(dom_attribute*) );
                if ( attrs != NULL )
                {
                    int i = 0;
                    if ( di->elem.attrs != NULL )
                    {
                        while ( di->elem.attrs[i] != NULL )
                            attrs[i] = di->elem.attrs[i++];
                    }
                    if ( di->elem.attrs != NULL )
                        free( di->elem.attrs );
                    di->elem.attrs = attrs;
                    di->elem.allocated += BLOCK_SIZE;
                }
                else
                    res = 0;
            }
            di->elem.attrs[di->elem.used++] = attr;
        }
        else
            res = 0;
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
    {
        if ( !isdigit(value[i]) )
            return 0;
    }
    return 1;
}
/**
 * Is this key a binary string (don't convert to integer)
 * @param key the key to test
 * @return 1 if it is a binary string
 */
static int dom_is_binary( char *key )
{
    return strcmp(key,"versions")==0;
}
/**
 * Write an attribute to disk
 * @param attr the attribute to write out
 * @param dst the destination file
 * @param final if 1 this is the last attribute
 * @return 1 if it worked else 0
 */
static int dom_write_attribute( dom_attribute *attr, int depth, FILE *dst, 
    int final )
{
    int n,res = dom_write_spaces( dst, depth );
    if ( res )
    {
        char *fmt;
        int should_be,n;
        if ( (!dom_is_boolean(attr->value) && !dom_is_integer(attr->value)) 
            || dom_is_binary(attr->key) )
        {
            fmt = (final)?fmt_quoted_final:fmt_quoted;
            n = fprintf(dst, fmt, attr->key,attr->value);
            should_be = (strlen(fmt)+strlen(attr->key)+strlen(attr->value)-4);
            res = n==should_be;
        }
        else
        {
            fmt = (final)?fmt_unquoted_final:fmt_unquoted;
            n = fprintf(dst, fmt, attr->key,attr->value);
            should_be = (strlen(fmt)+strlen(attr->key)+strlen(attr->value)-4);
            res = n == should_be;
        }
    }
    return res;
}
/**
 * Write a start tag to disk
 * @param di the dom item in question
 * @param pa the parent object/array
 * @param dst the file to write to
 * @param depth the number of spaces to indent
 * @param log the log to record errors in
 * @return 1 if it worked else 0 
 */
static int dom_item_start( dom_item *di, dom_item *pa, FILE *dst, int depth )
{
    int n,res = dom_write_spaces( dst, depth );
    if ( res )
    {
        char *fmt;
        if ( di->kind == OBJECT_KIND )
            fmt = "\"%s\": {\n";
        else
            fmt = "\"%s\": [\n";
        // start-tag
        if ( pa==NULL || pa->kind==ARRAY_KIND )
        {
            n = fprintf(dst, "{\n");
            if ( n != 2 )
                res = 0;
        }
        else
        {
            n = fprintf( dst, fmt, di->name );
            if ( n != 6+strlen(di->name) )
                res = 0;
        }
        int i = 0;
        if ( res && di->kind == OBJECT_KIND )
        {
            if ( di->elem.attrs != NULL )
            {
                while ( res && di->elem.attrs[i] != NULL )
                {
                    int final = (di->elem.attrs[i+1]==NULL
                        &&di->elem.text==NULL
                        &&di->elem.children==NULL);
                    res = dom_write_attribute( di->elem.attrs[i++], 
                        depth+INDENT, dst, final );
                }
            }
        }
    }
    return res;
}
/**
 * Write the end tag
 * @param de the item
 * @param dst the destination file
 * @param final last in series: no comma
 * @return 1 if it worked else 0
 */
static int dom_item_end( dom_item *di, FILE *dst, int depth, int final )
{
    int n,res=1;
    res = dom_write_spaces( dst, depth );
    if ( res )
    {
        const char *fmt;
        if ( final )
        {
            if ( di->kind == OBJECT_KIND )
                fmt = "}\n";
            else
                fmt = "]\n";
            n = fwrite( fmt, 1, 2, dst );
            if ( n != 2 )
                res = 0;
        }
        else 
        {
            if ( di->kind == OBJECT_KIND )
                fmt = "},\n";
            else
                fmt = "],\n";
            n = fwrite( fmt, 1, 3, dst );
            if ( n != 3 )
                res = 0;
        }
    }
    return res;
}
/**
 * Resize a dynamic string
 * @param dst the string to resize
 * @param pos the current length of the string
 * @param slen the length of the string to add to it
 * @param limit the current maximum length of dst
 * @return the resized string or NULL on failure
 */
static char *resize( char *dst, int pos, int slen, int *limit )
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
 * Write out an entire object and its children
 * @param di the item to write out
 * @param pa the parent item
 * @param indent the number of space to indent the object
 * @param dst the file to write to
 * @param final 1 if this is the last in a series
 * @return 1 if it worked else 0
 */
static int dom_write_item( dom_item *di, dom_item *pa, int indent, FILE *dst, 
    int final )
{
    int res = dom_item_start( di, pa, dst, indent );
    if ( res )
    {
        if ( di->kind == OBJECT_KIND )
        {
            if ( di->elem.children != NULL )
            {
                dom_item *child = di->elem.children;
                while ( res && child != NULL )
                {
                    res = dom_write_item( child, di, indent+INDENT, dst, 
                        final );
                    child = child->next;
                }
            }
            else if ( di->elem.text != NULL )
            {
                char *escaped = dom_escape(di->elem.text);
                if ( escaped != NULL )
                {
                    res = dom_write_spaces(dst,indent+4);
                    if ( res )
                    {
                        int n = fprintf(dst,"\"text\": \"%s\"\n",escaped );
                        if ( n != strlen(escaped)+11 )
                            res = 0;
                    }
                    free( escaped );
                }
            }
        }
        else
        {
            int i,nitems = dyn_array_size( di->array.items );
            for ( i=0;i<nitems;i++ )
            {
                dom_item *item = dyn_array_get( di->array.items, i );
                res = dom_write_item( item, di, indent+INDENT, dst, 
                    i==nitems-1 );
                if ( !res )
                    break;
            }
        }
    }
    if ( res )
        res = dom_item_end( di, dst, indent, final );
    return res;
}
/**
 * Write a dom out to disk
 * @param root the root object
 * @param dst the destination file
 * @return 1 if it worked else 0
 */
int dom_externalise( dom_item *root, FILE *dst )
{
    return dom_write_item( root, NULL, 0, dst, 1 );
}
/**
 * Add a child to this item. Must NOT have any text.
 * @param parent the parent to get the new child
 * @param child the child to add
 * @return 1 if all was OK else 0
 */
int dom_add_child( dom_item *parent, dom_item *child )
{
    int res = 1;
    if ( parent->kind == OBJECT_KIND )
    {
        if ( parent->elem.text != NULL )
            res = 0;
        else if ( parent->elem.children == NULL )
            parent->elem.children = child;
        else
        {
            dom_item *sibling = parent->elem.children;
            while ( sibling->next != NULL )
                sibling = sibling->next;
            sibling->next = child;
        }
    }
    else
    {
        res = dyn_array_add( parent->array.items, child );
    }
    return res;
}
/**
 * Add some text to a node. Must NOT have any children.
 * @param di the item must be an object
 * @param text the text to add
 * @return 1 if it worked else 0
 */
int dom_add_text( dom_item *di, char *text )
{
    int res = 1;
    if ( di->kind == OBJECT_KIND )
    {
        if ( di->elem.children == NULL )
            di->elem.text = strdup(text);
        if ( di->elem.text == NULL )
            res = 0;
    }
    else
        res = 0;
    return res;
}
#ifdef DOM_TEST
int main( int argc, char **argv )
{
    dom_item *root = dom_object_create( "root" );
    dom_add_attribute( root, dom_attribute_create("encoding","utf-8") );
    dom_item *child = dom_object_create("child");
    dom_add_child( root, child );
    dom_add_attribute( child, dom_attribute_create("bananas","4"));
    dom_add_attribute( child, dom_attribute_create("pears","2"));
    dom_item *grandchild1 = dom_object_create("grandchild" );
    dom_item *grandchild2 = dom_object_create("grandchild");
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