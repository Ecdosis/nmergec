if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
 else
  LIBSUFFIX=so
fi
gcc -g -I../shared/include -I/usr/local/include -I. -I../../include \
-I../../mvd/include -fPIC ../shared/src/*.c *.c -shared -lmvd -licuuc \
-o lib${PWD##*/}.$LIBSUFFIX
if [ ! -d "/usr/local/lib/nmerge-plugins" ]; then
    mkdir -p /usr/local/lib/nmerge-plugins
fi
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/nmerge-plugins/
