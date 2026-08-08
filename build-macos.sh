set -x

clang -o plugin.so plugin.c -I./mpv/include -shared -fPIC -Wl,-undefined,dynamic_lookup