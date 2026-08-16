set -x

cc -o shmshim.o -Wall -O3 -c shmshim.c
cc -o plugin.dll -Wall -Wno-format plugin.c -I./mpv/include -shared
cc -o example-c.exe -Wall example.c shmshim.o
