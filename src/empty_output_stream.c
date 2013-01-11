/*
 *  NMerge is Copyright 2009 Desmond Schmidt
 * 
 *  This file is part of NMerge. NMerge is a Java library for merging 
 *  multiple versions into multi-version documents (MVDs), and for 
 *  reading, searching and comparing them.
 *
 *  NMerge is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "output_stream.h"
#include "empty_output_stream.h"
/**
 * This class is used to throw away output from the MvdTool during testing
 * @author Desmond Schmidt 3/5/09
 */
struct empty_output_stream_struct
{
    output_stream *os;
    int received;
};
/**
 * Find out if we received any data
 * @param eos the empty output stream instance
 * @return the number of bytes received since last flush
 */
int printedBytes( empty_output_stream *eos )
{
    return eos->received;
}
/**
 * Does nothing
 * @param eos the eos instance
 */
void empty_output_stream_close( empty_output_stream *eos )
{
}
/**
 * Don't use flush because the system calls it unexpectedly
 * @param eos the eos instance
 */
void empty_output_stream_clear( empty_output_stream *eos )
{
    eos->received = 0;
}
/**
 * Does nothing
 * @param eos the eos instance
 */
void empty_output_stream_flush( empty_output_stream *eos )
{
}
/**
 * Does nothing
 * @param eos the eos instance
 * @param b the data
 * @param len the length of the data to write
 */
void empty_output_stream_write_bytes( empty_output_stream *eos, char *b, 
    int len )
{
    eos->received += len;
}
/**
 * Does nothing
 * @param eos the eos instance
 * @param b the data
 * @param off the start offset in the data.
 * @param len the number of bytes to write. 
 */
void empty_output_stream_write_range( empty_output_stream *eos, char *b, 
    int off, int len )
{
    eos->received += len;
}
/**
 * Does nothing
 * @param b a byte of data to write
 */
void empty_output_stream_write( empty_output_stream *eos, int b )
{
    eos->received++;
}


