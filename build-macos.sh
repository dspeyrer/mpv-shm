set -x

clang -Wall -Wno-format -o plugin.so plugin.c -I./mpv/include -shared -fPIC -Wl,-undefined,dynamic_lookup -v
clang -Wall -o example example.c
