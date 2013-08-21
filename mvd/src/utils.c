#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "unicode/uchar.h"
#include "hashmap.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
    
/**
 * convert an integer into a temporary string
 * @param value the int value to stringify
 * @param buf the buffer to write to
 * @param len the number of bytes in buf
 * @return buf, with the value written into it
 */
char *itoa( int value, char *buf, int len )
{
    snprintf(buf,len,"%d",value);
    return buf;
}
UChar *u_strdup(UChar *in) 
{
    uint32_t len = u_strlen(in) + 1;
    UChar *result = malloc(sizeof(UChar) * len);
    if ( result != NULL )
        u_memcpy(result, in, len);
    return result;
}
/**
 * Duplicate a range of an existing ustring
 * @param in the input ustring
 * @param len the desired length
 * @return the new string allocated or NULL
 */
UChar *u_strndup(UChar *in, int len)
{
    UChar *result = malloc(sizeof(UChar) * (len+1));
    if ( result != NULL )
        u_memcpy(result, in, len);
    return result;
}
/**
 * Convert a utf-16 string of digits to a number
 * @param str a UTF-16 string
 * @return its value
 */
int u_atoi( UChar *str )
{
    int i,len = u_strlen( str );
    int value = 0;
    for ( i=0;i<len;i++ )
    {
        value *= 10;
        if ( str[i]>=48&&str[i]<=57 )
            value += str[i]-48;
        // ignore other ascii codes
    }
    return value;
}
/**
 * Generate a 16 byte random UTF-16 string for testing
 * @return the allocated string
 */
UChar *random_str()
{
    static UChar alphabetti[26] = {'a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p',
	'q','r','s','t','u','v','w','x','y','z'};
    UChar *str = calloc( 16, sizeof(UChar) );
    if ( str != NULL )
    {
        int i;
        for ( i=0;i<15;i++ )
            str[i] = alphabetti[rand()%26];
        str[15] = 0;
    }
    return str;
}
char *u_print( UChar *ustr, char *buf, int n )
{
    int i,len = u_strlen( ustr );
    int limit = n-1;
    for ( i=0;i<len&&i<limit;i++ )
        buf[i] = (char)ustr[i];
    buf[i] = 0;
    return buf;
}
void lowercase( char *str )
{
    int i,len = strlen(str);
    for ( i=0;i<len;i++ )
        str[i] = tolower(str[i]);
}
/**
 * Do a quick and dirty ascii to uchar conversion
 * @param str the ascii string
 * @param u_str the destination uchar string
 * @param u_len the number of uchars in u_str
 */
void ascii_to_uchar( char *str, UChar *u_str, int u_len )
{
    int slen = strlen( str );
    if ( slen+1 <= u_len )
    {
        int i;
        for ( i=0;i<slen;i++ )
            u_str[i] = (UChar)str[i];
        u_str[slen] = 0;
    }
    else
        fprintf(stderr,"utils: UChar buffer too short\n");
}
void calc_ukey( UChar *u_key, long value, int len )
{
    memset( u_key, 0, len*sizeof(UChar) );
    char *key = calloc( len, 1 );
    if ( key != NULL )
    {
        snprintf( key, len, "%lx", value );
        ascii_to_uchar( key, u_key, len );
        free( key );
    }
    else
    {
        fprintf(stderr,"mvd: failed to create ukey\n");
        u_key[0] = 0;
    }
}
/**
 * Strip quotation marks from the end of a string
 * @param str the string to strip
 */
void strip_quotes( char *str )
{
    int i = 0;
    while ( str[i] != 0 && (str[i] == '"' || str[i]=='\'') )
        i++;
    int j = strlen(str)-1;
    while ( j>=0 && (str[j] == '"' || str[j]=='\'') )
        j--;
    memcpy( str, &str[i], j-i+1 );
    str[j-i+1] = 0;
}
/**
 * Store a key-value pair in a map. Free key but not value.
 * @param hm the map
 * @param key the utf-8 key
 * @param value the utf-8 value
 * @return 1 if it worked else 0
 */
static int store_key( hashmap *hm, char *key, char *value )
{
    // key and value are non-NULL on entry
    int nbytes,res = 1;
    int u_len = measure_from_encoding( key, strlen(key), "utf-8" );
    UChar *u_key = calloc( u_len+1, sizeof(UChar) );
    if ( u_key != NULL )
    {
        nbytes = convert_from_encoding( key, strlen(key), u_key, u_len+1, 
            "utf-8" );
        if (nbytes != u_len*2 )
            res = 0;
    }
    if ( res )
        res = hashmap_put( hm, u_key, value );
    // was duplicated
    if ( u_key != NULL )
        free( u_key );
    return res;
}
/**
 * Parse the options string into a map of key-value pairs
 * @param options the raw options string name=value. values are allocated.
 * @return a map of key value pairs
 */
hashmap *parse_options( char *options )
{
    hashmap *map = hashmap_create( 6, 0 );
    if ( map != NULL )
    {
        char *str = strdup( options );
        if ( str != NULL )
        {
            int i,len = strlen( str );
            int state = 0;
            int key_start = 0;
            int value_start = 0;
            char *key = NULL;
            char *value = NULL;
            int res = 1;
            for ( i=0;i<len;i++ )
            {
                switch ( state )
                {
                    case 0:// building key
                        if ( str[i]=='=' )
                        {
                            str[i] = 0;
                            key = strdup( &str[key_start] );
                            state = 1;
                        }
                        break;
                    case 1://looking for value
                        if ( str[i]=='"' )
                        {
                            value_start = i+1;
                            state = 2;
                        }
                        else if ( isalnum(str[i]) )
                        {
                            value_start = i;
                            state = 3;
                        }
                        break;
                    case 2: //parsing quoted value
                        if ( str[i]=='"')
                        {
                            str[i]=0;
                            value = strdup(&str[value_start]);
                            if ( key != NULL && value != NULL )
                            {
                                res = store_key(map,key,value);
                                free( key );
                            }
                            state = 4;
                        }
                        break;
                    case 3:// parsing unquoted value
                        if ( isspace(str[i])||i==len-1 )
                        {
                            if ( isspace(str[i]) )
                                str[i] = 0;
                            value = strdup(&str[value_start]);
                            if ( key != NULL && value != NULL )
                            {
                                res = store_key(map,key,value);
                                free( key );
                            }
                            state = 4;
                        }
                        break;
                    case 4: // looking for key start
                        if ( !isspace(str[i]) )
                        {
                            key_start = i;
                            state = 0;
                        }
                        break;
                }
                if ( !res ) 
                    break;
            }
            free( str );
        }
    }
    return map;
}
/**
 * Get a file size
 * @param file_name the file path local to WD
 * @return its size in bytes
 */
int file_size( const char *file_name )
{
    FILE *fp = fopen( file_name, "r" );
    long sz = -1;
    if ( fp != NULL )
    {
        fseek(fp, 0L, SEEK_END);
        sz = ftell(fp);
        fclose( fp );
    }
    return (int) sz;
}
/**
 * Read a file and return its contents as an allocated buffer
 * @param file the file's path
 * @param len VAR param to set to the length of the contents
 * @return the file's contents, caller to free
 */
char *read_file( char *file, int *len )
{
	FILE *fp = fopen(file,"r");
	char *buf = NULL;
	int res = 0;
	if ( fp == NULL )
	{
		fprintf(stderr, "couldn't open %s\n", file);
        return NULL;
	}
	*len = file_size(file);
	if ( *len > 0 )
	{
		buf = malloc( (*len)+1 );
		if ( buf != NULL )
			res = fread( buf, 1, *len, fp );
		if ( res != *len )
		{
			free( buf );
			buf = NULL;
		}
	}
	else
	{
		fprintf(stderr,"file %s length is 0\n", file);
	}
	return buf;
}
#ifdef MVD_TEST
static UChar ustr[] = {'b','a','n','a','n','a',0};
static UChar nstr[] = {'2','4','6',0};
static UChar xstr[] = {'2','3','4','5',0};
static UChar size_ustr[] = {'s','i','z','e',0};
static UChar name_ustr[] = {'n','a','m','e',0};
static UChar length_ustr[] = {'l','e','n','g','t','h',0};
void test_utils( int *passed, int *failed )
{
    char buf[32];
    UChar ubuf[8];
    itoa( 238, buf, 32 );
    if ( strcmp(buf,"238")!=0 )
    {
        fprintf(stderr,"util: failed to write int to string\n");
        (*failed)++;
    }
    else
        (*passed)++;
    UChar *u_cpy = u_strdup(ustr);
    if ( u_cpy==NULL||u_strcmp(u_cpy,ustr)!=0 )
    {
        (*failed)++;
        fprintf(stderr,"util: failed to copy unicode str\n");
    }
    else
        (*passed)++;
    if ( u_cpy != NULL )
        free( u_cpy );
    u_cpy = u_strndup(ustr, 5);
    if ( u_cpy != NULL && u_strncmp(u_cpy,ustr,5)==0 )
        (*passed)++;
    else
    {
        (*failed)++;
        fprintf(stderr,"util: failed to n-copy unicode str\n");
    }
    if ( u_cpy != NULL )
        free( u_cpy );
    int n = u_atoi( nstr );
    if ( n != 246 )
    {
        fprintf(stderr,"utils: failed to convert integer from string\n");
        (*failed)++;
    }
    else
        (*passed)++;
    UChar *rstr = random_str();
    if ( u_strlen(rstr)!= 15 )
    {
        fprintf(stderr,"utils: random string length wrong\n");
        (*failed)++;
    }
    else
        (*passed)++;
    u_print( ustr, buf, 32 );
    if ( strcmp(buf,"banana")!=0 )
    {
        fprintf(stderr,"utils: failed to reduce u_string to cstring\n");
        (*failed)++;
    }
    else
        (*passed)++;
    strcpy( buf, "BaNaNa");
    lowercase( buf );
    if ( strcmp(buf,"banana")!=0 )
    {
        fprintf(stderr,"utils: failed to lowercase\n");
        (*failed)++;
    }
    else
        (*passed)++;
    ascii_to_uchar( "banana", ubuf, 8 );
    if ( u_strcmp(ubuf,ustr)!=0 )
    {
        fprintf(stderr,"utils: failed to convert to unicode\n");
        (*failed)++;
    }
    else
        (*passed)++;
    calc_ukey( ubuf, 0x2345, 8 );
    if ( u_strcmp(ubuf,xstr)!=0 )
    {
        fprintf(stderr,"utils: failed to create unicode key\n");
        (*failed)++;
    }
    else
        (*passed)++;
    strcpy( buf, "\"banana\'");
    strip_quotes( buf );
    if ( strcmp(buf,"banana")!=0 )
    {
        fprintf(stderr,"utils: failed to strip quotes\n");
        (*failed)++;
    }
    else
        (*passed)++;
    hashmap *hm = hashmap_create(5,0);
    if ( hm != NULL )
    {
        int res = store_key( hm, "banana", "milkshake" );
        if ( !res || !hashmap_contains(hm,ustr) )
        {
            fprintf(stderr,"utils: failed to store key\n");
            (*failed)++;
        }
        hashmap_dispose( hm, NULL );
    }
    hm = parse_options( "size=\"4\" length=\"23\" name=\"agent 86\"" );
    if ( hm != NULL )
    {
        char *size = hashmap_get(hm,size_ustr);
        char *name = hashmap_get(hm,name_ustr);
        char *length = hashmap_get(hm,length_ustr);
        if ( size!=NULL && atoi(size)==4 )
            (*passed)++;
        else
        {
            fprintf(stderr,"utils: failed to retrieve size key\n");
            (*failed)++;
        }
        if ( length!=NULL && atoi(length)==23 )
            (*passed)++;
        else
        {
            fprintf(stderr,"utils: failed to retrieve length key\n");
            (*failed)++;
        }
        if ( name!=NULL && strcmp(name,"agent 86")==0 )
            (*passed)++;
        else
        {
            fprintf(stderr,"utils: failed to retrieve name key\n");
            (*failed)++;
        }
        hashmap_dispose( hm, NULL );
    }
    int fsize = file_size( "tests/kinglear.mvd" );
    if ( fsize != 20688 )
    {
        fprintf(stderr,"utils: file_size incorrect\n");
        (*failed)++;
    }
    else
        (*passed)++;
    int len;
    char *contents = read_file( "tests/can_1316_01.txt", &len );
    if ( contents != NULL && len == 2402 )
        (*passed)++;
    else
    {
        (*failed)++;
        fprintf(stderr,"utils: failed to read file\n");
    }
    if ( contents != NULL )
        free( contents );
}
#endif