#ifdef MVD_ADD_TEST
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "node.h"
#include "pos.h"
#include "suffixtree.h"
#include "version.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"
#include "hashmap.h"
#include "utils.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
static char *folder;
static char output[SCRATCH_LEN];
                    
/**
 * Get the length of an open file
 * @param fp an open FILE handle
 * @return file length if successful, else 0
 */
static int file_length( FILE *fp )
{
	int length = 0;
    int res = fseek( fp, 0, SEEK_END );
	if ( res == 0 )
	{
		long long_len = ftell( fp );
		if ( long_len > INT_MAX )
        {
			fprintf( stderr,"mvd_add: file too long: %ld", long_len );
            length = res = 0;
        }
		else
        {
            length = (int) long_len;
            if ( length != -1 )
                res = fseek( fp, 0, SEEK_SET );
            else
                res = 1;
        }
	}
	if ( res != 0 )
    {
		fprintf(stderr, "mvd_add: failed to read file. error %s\n",
            strerror(errno) );
        length = 0;
    }
	return length;
}
/**
 * Read a file
 * @param file the path to the file
 * @param flen update with the length of the file
 * @return NULL on failure else the allocated text content
 */
static char *read_file( char *file, int *flen )
{
    char *data = NULL;
    FILE *fp = fopen( file, "r" );
    if ( fp != NULL )
    {
        int len = file_length( fp );
        data = malloc( len+1 );
        if ( data != NULL )
        {
            int read = fread( data, 1, len, fp );
            if ( read != len )
            {
                fprintf(stderr,"mvd_add: failed to read %s\n",file);
                free( data );
                data = NULL;
                *flen = 0;
            }
            else
            {
                data[len] = 0;
                *flen = len;
            }
        }
        else
            fprintf(stderr,"mvd_add: failed to allocate file buffer\n");
        fclose( fp );
    }
    return data;
}
static char *create_path( char *dir, char *file )
{
    int len1 = strlen( dir );
    int len2 = strlen( file );
    char *path = malloc( len1+len2+2 );
    if ( path != NULL )
    {
        strcpy( path, dir );
        strcat( path, "/" );
        strcat( path, file );
    }
    return path;
}
int write_mvd( MVD *mvd, char *file )
{
    int res = 0;
    int size = mvd_datasize( mvd, 1 );
    unsigned char *data = malloc( size );
    if ( data != NULL )
    {
        int res = mvd_serialise( mvd, data, size, 1 );
        if ( res )
        {
            FILE *dst = fopen( file, "w" );
            if ( dst != NULL )
            {
                int nitems = fwrite( data, 1, size, dst );
                if ( nitems == size )
                    res = 1;
                fclose( dst );
            }
        }
    }
    return res;
}
/**
 * Read a directory
 * @return number of files found or 0 on failure
 */
static int read_dir( char *folder )
{
    int n_files = 0;
    DIR *dir;
    int res = 1;
    struct dirent *ent;
    UChar desc[6];
    MVD *mvd=mvd_create();
    mvd_set_encoding(mvd, "utf-8");
    ascii_to_uchar( "test", desc, 6 );
    mvd_set_description( mvd, desc);
    if ((dir = opendir(folder)) != NULL) 
    {
        while ((ent = readdir(dir)) != NULL && res) 
        {
            int flen;
            if ( strcmp(ent->d_name,".")!=0&&strcmp(ent->d_name,"..")!=0 )
            {
                char *path = create_path(folder,ent->d_name);
                if ( path != NULL )
                {
                    char *txt = read_file( path, &flen );
                    if ( txt == NULL )
                        break;
                    else
                    {
                        char options[128];
                        options[0] = 0;
                        strcat( options, "vid=" );
                        strcat( options, ent->d_name );
                        strcat( options, " encoding=utf-8" );
                        strcat( options, " description=test" );
                        res = process( &mvd, options, output, txt, flen );
                        n_files++;
                        printf("%s",(char*)output);
                        free( txt );
                    }
                    free( path );
                }
            }
        }
        closedir( dir );
    }
    else
        fprintf(stderr,"test: failed to open directory %s\n",folder);
    write_mvd( mvd, "test.mvd" );
    return n_files;
}
// arguments: folder of text files
int main( int argc, char **argv )
{
    if ( argc == 2 )
    {
        int res = read_dir( argv[1] );
    }
    else
        printf("mvd_add: usage ./mvd_add <dir>\n");
}
#endif
