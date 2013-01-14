/* 
 * File:   b64.h
 * Author: desmond
 *
 * Created on January 14, 2013, 9:38 AM
 */

#ifndef B64_H
#define	B64_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef MVD_TEST
int test_b64( int *passed, int *failed );
#endif
void b64_encode( const unsigned char *data, size_t input_len, 
    char *output, size_t output_len );
size_t b64_encode_buflen( size_t input_len );
size_t b64_decode_buflen( size_t input_len );
void b64_decode(const char *data, size_t input_len, 
    unsigned char *output, size_t output_len );
#ifdef	__cplusplus
}
#endif

#endif	/* B64_H */

