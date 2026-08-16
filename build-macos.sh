set -x

cc -o shmshim.o -Wall -O3 -c shmshim.c
cc -o plugin.so -Wall -Wno-format plugin.c -I./mpv/include -shared -fPIC -Wl,-undefined,dynamic_lookup
cc -o example-c -Wall example.c shmshim.o
rustc -o example-rs example.rs -l shmshim.o
