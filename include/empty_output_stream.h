/* 
 * File:   empty_output_stream.h
 * Author: desmond
 *
 * Created on January 11, 2013, 1:20 PM
 */

#ifndef EMPTY_OUTPUT_STREAM_H
#define	EMPTY_OUTPUT_STREAM_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct empty_output_stream_struct empty_output_stream;
int printedBytes( empty_output_stream *eos );
void empty_output_stream_close( empty_output_stream *eos );
void empty_output_stream_clear( empty_output_stream *eos );
void empty_output_stream_flush( empty_output_stream *eos );
void empty_output_stream_write_bytes( empty_output_stream *eos, char *b, int len );
void empty_output_stream_write_range( empty_output_stream *eos, char *b, 
    int off, int len );
void empty_output_stream_write( empty_output_stream *eos, int b );


#ifdef	__cplusplus
}
#endif

#endif	/* EMPTY_OUTPUT_STREAM_H */

