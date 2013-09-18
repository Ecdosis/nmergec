/*
 *  NMergeC is Copyright 2013 Desmond Schmidt
 * 
 *  This file is part of NMergeC. NMergeC is a C commandline tool and 
 *  static library and a collection of dynamic libraries for merging 
 *  multiple versions into multi-version documents (MVDs), and for 
 *  reading, searching and comparing them.
 *
 *  NMergeC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NMergeC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "benchmark.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
/**
 * Get the current memory usage of this process (unreliable)
 * @return the fixed memory in use by this process
 */
long get_mem_usage()
{
    int who= RUSAGE_SELF;
    struct rusage usage;
    struct rusage *p=&usage;

    getrusage(who,p);
    return usage.ru_maxrss;
}
/**
 * Get the current time in microseconds
 * @return the time in microseconds since the epoch
 */
int64_t epoch_time()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return tv.tv_sec*1000000+tv.tv_usec;
}
