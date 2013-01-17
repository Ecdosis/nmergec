#include <stdlib.h>
#include <stdio.h>
#include "mvd/MVD.h"
#include "dyn_array.h"
#include "bitset.h"
#define DEFAULT_VERSION_SIZE 32
#define DEFAULT_PAIRS_SIZE 4096
#define DEFAULT_ENCODING "UTF-8"
struct MVD_struct
{
    dyn_array *versions;	// id = position in table+1
	dyn_array *pairs;
	char *description;
	char *encoding;
};
/**
 * Create an empty MVD
 * @return the MVD or NULL
 */
MVD *mvd_create()
{
    MVD mvd = calloc( 1, sizeof(MVD) );
    if ( mvd != NULL )
    {
        mvd->versions = dyn_array_create( DEFAULT_VERSION_SIZE );
        mvd->pairs = dyn_array_create( DEFAULT_PAIRS_SIZE );
        mvd->description = "";
        mvd->encoding = DEFAULT_ENCODING;
        if ( mvd->versions==NULL||mvd->pairs==NULL )
        {
            mvd_dispose( mvd );
            mvd = NULL;
        }
    }
    else
    {
        fprintf(stderr,"MVD: failed to allocate object\n");
    }
    return mvd;
}
/**
 * Dispose of a perhaps partly allocate mvd
 * @param mvd the mvd in question
 */
void mvd_dispose( MVD *mvd )
{
    if ( mvd->versions == NULL )
    {
        int i;
        for ( i=0;i<dyn_array_size(mvd->versions);i++ )
        {
            char *v = dyn_array_get(mvd->versions,i);
            free(v);
        }
        dyn_array_dispose( mvd->versions );
    }
    if ( mvd->pairs != NULL )
    {
        int i;
        for ( i=0;i<dyn_array_size(mvd->pairs);i++ )
        {
            pair *p = dyn_array_get(mvd->pairs,i);
            pair_dispose(p);
        }
        dyn_array_dispose( mvd->pairs );
    }
    if ( mvd->description != NULL && strlen(mvd->description)>0 )
        free( mvd->description );
    if ( strcmp(mvd->encoding,DEFAULT_ENCODING) != 0 )
        free( mvd->encoding );
}
int mvd_datasize( MVD *mvd )
{
    return 1;
}
int mvd_serialise( MVD *mvd, unsigned char *data )
{
    return 1;
}