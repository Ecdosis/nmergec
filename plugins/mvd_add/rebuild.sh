if [ `uname` = "Darwin" ]
 then
  LIBSUFFIX=dylib
  RTLIB=
 else
  RTLIB=-lrt
  LIBSUFFIX=so
fi
MFILES="../../src/mvd/mvd.c ../../src/bitset.c ../../src/dyn_array.c ../../src/hashmap.c ../../src/hsieh.c ../../src/link_node.c ../../src/utils.c ../../src/mvd/group.c ../../src/mvd/pair.c ../../src/mvd/serialiser.c ../../src/mvd/version.c"
gcc -I/usr/local/include -I. -Iinclude -I../../include -I../../include/mvd -fPIC src/*.c $MFILES -shared -L/usr/local/lib -licuuc $RTLIB -o lib${PWD##*/}.$LIBSUFFIX
if [ ! -d "/usr/local/lib/nmerge-plugins" ]; then
    mkdir -p /usr/local/lib/nmerge-plugins
fi
cp lib${PWD##*/}.$LIBSUFFIX /usr/local/lib/nmerge-plugins/
