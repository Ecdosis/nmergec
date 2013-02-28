if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
  RTLIB=
 else
  RTLIB=-lrt
  LIBSUFFIX=so
fi
gcc -I../shared/include -I/usr/local/include -I. -Iinclude -I../../include -I../../mvd/include -fPIC ../shared/src/*.c src/*.c -shared -lmvd -licuuc -o lib${PWD##*/}.$LIBSUFFIX
if [ ! -d "/usr/local/lib/nmerge-plugins" ]; then
    mkdir -p /usr/local/lib/nmerge-plugins
fi
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/nmerge-plugins/
