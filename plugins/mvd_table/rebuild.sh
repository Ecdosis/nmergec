if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
  RTLIB=
 else
  RTLIB=-lrt
  LIBSUFFIX=so
fi
gcc -I. -I../../include -I../../mvd/include -fPIC *.c -shared $RTLIB -o lib${PWD##*/}.$LIBSUFFIX
if [ ! -d "/usr/local/lib/nmerge-plugins" ]; then
    mkdir -p /usr/local/lib/nmerge-plugins
fi
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/nmerge-plugins/
