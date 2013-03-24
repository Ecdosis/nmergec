/* 
 * File:   mvd.h
 * Author: desmond
 *
 * Created on January 14, 2013, 7:28 AM
 */

#ifndef MVD_H
#define	MVD_H

#ifdef	__cplusplus
extern "C" {
#endif
typedef struct MVD_struct MVD;
MVD *mvd_create();
void *mvd_dispose( MVD *mvd );
int mvd_datasize( MVD *mvd, int old );
int mvd_serialise( MVD *mvd, unsigned char *data, int len, int old );
void mvd_set_bitset_size( MVD *mvd, int setSize );
int mvd_add_version( MVD *mvd, version *v );
int mvd_get_set_size( MVD *mvd );
char *mvd_get_encoding( MVD *mvd );
dyn_array *mvd_get_pairs( MVD *mvd );
int mvd_count_versions( MVD *mvd );
int mvd_count_pairs( MVD *mvd );
int mvd_set_description( MVD *mvd, UChar *description );
int mvd_set_encoding( MVD *mvd, char *encoding );
int mvd_add_pair( MVD *mvd, pair *tpl2 );
#define MVD_MAGIC_LEN 4
#define MVD_MAGIC_OLD_STR "\336\255\300\336"
#define MVD_MAGIC_NEW_STR "\140\015\300\336"

#ifdef MVD_TEST
int mvd_test_versions( MVD *mvd );
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* MVD_H */

