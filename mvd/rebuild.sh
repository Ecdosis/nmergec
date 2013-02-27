#!/bin/bash
if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
  RTLIB=
 else
  RTLIB=-lrt
  LIBSUFFIX=so
fi
gcc -I/usr/local/include -Iinclude -fPIC src/*.c -shared -L/usr/local/lib -licuuc $RTLIB -o lib${PWD##*/}.$LIBSUFFIX
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/
