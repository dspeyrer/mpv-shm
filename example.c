#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
#endif

int main(int argc, char **argv) {
	int ret = 1;

	if (argc != 3) {
		printf("usage: %s <file_path> <shm_name>\n", argv[0]);
		return 1;
	}

	FILE *f = fopen(argv[1], "rb");

	if (f == NULL) {
		printf("fopen(): %s\n", strerror(errno));
		return 1;
	}

	if (fseeko(f, 0, SEEK_END) < 0) {
		printf("fseek(): %s\n", strerror(errno));
		goto cleanup_file;
	}

	off_t len = ftello(f);

	if (len < 0) {
		printf("ftell(): %s\n", strerror(errno));
		goto cleanup_file;
	}

	rewind(f);

#ifdef _WIN32
	HANDLE shm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, len, argv[2]);

	if (shm == NULL) {
		printf("CreateFileMapping(): %lu\n", GetLastError());
		goto cleanup_file;
	}

	char *ptr = MapViewOfFile(shm, FILE_MAP_ALL_ACCESS, 0, 0, len);

	if (ptr == NULL) {
		printf("MapViewOfFile(): %lu\n", GetLastError());
		goto cleanup_shm;
	}
#else
	size_t shm_name_len = strlen(argv[2]);
	char *shm_name = malloc(shm_name_len + 2);
	shm_name[0] = '/';
	memcpy(shm_name + 1, argv[2], shm_name_len + 1);

	shm_unlink(shm_name);
	int shm = shm_open(shm_name, O_RDWR | O_CREAT);

	if (shm < 0) {
		printf("shm_open(): %s\n", strerror(errno));
		goto cleanup_shm_name;
	}

	int res = ftruncate(shm, len);

	if (res < 0) {
		printf("ftruncate(): %s\n", strerror(errno));
		goto cleanup_shm;
	}

	char *ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

	if (ptr == MAP_FAILED) {
		printf("mmap(): %s\n", strerror(errno));
		goto cleanup_shm;
	}
#endif

	size_t n = fread(ptr, len, 1, f);

	if (n == 0) {
		printf("fread(): 0\n");
		goto cleanup_map;
	}

	printf("map available at shm://0+%llx/%s\n", len, argv[2]);
	printf("Press any key to exit...\n");

	getc(stdin);

	ret = 0;
cleanup_map:
#ifdef _WIN32
	UnmapViewOfFile(ptr);
cleanup_shm:
	CloseHandle(shm);
#else
	munmap(ptr, len);
cleanup_shm:
	close(shm);
	shm_unlink(shm_name);
cleanup_shm_name:
	free(shm_name);
#endif
cleanup_file:
	fclose(f);
	return ret;
}
