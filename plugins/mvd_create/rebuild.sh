if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
 else
  LIBSUFFIX=so
fi
gcc -I. -I../../include -I../../mvd/include -I../shared/include -fPIC ../shared/src/plugin_log.c *.c -shared -lmvd -o lib${PWD##*/}.$LIBSUFFIX
if [ ! -d "/usr/local/lib/nmerge-plugins" ]; then
    mkdir -p /usr/local/lib/nmerge-plugins
fi
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/nmerge-plugins/
