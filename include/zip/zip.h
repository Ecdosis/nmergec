/* 
 * File:   zip.h
 * Author: desmond
 *
 * Created on January 14, 2013, 4:02 PM
 */

#ifndef ZIP_H
#define	ZIP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
int zip_deflate( unsigned char *src, int src_len, char_buf *buf );
int zip_inflate( unsigned char *src, int src_len, char_buf *buf );
#ifdef MVD_TEST
void test_zip( int *passed, int *failed );
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* ZIP_H */

