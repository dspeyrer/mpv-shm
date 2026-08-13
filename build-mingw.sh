gcc -Wall -Wno-format -o plugin.dll plugin.c -I./mpv/include -shared
gcc -Wall -o example.exe example.c
