#include "shmshim.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

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

	uint8_t **ptr = shm_shim_create((const uint8_t *)argv[2], strlen(argv[2]), len);

	if (!ptr) {
		printf("shm_shim_create(): ERROR\n");
		goto cleanup_file;
	}

	size_t n = fread(*ptr, len, 1, f);

	if (n == 0) {
		printf("fread(): 0\n");
		goto cleanup_shm;
	}

	printf("map available at shm://%llx/%s\n", len, argv[2]);
	printf("Press any key to exit...\n");

	getc(stdin);

	ret = 0;

cleanup_shm:
	shm_shim_destroy(ptr);

cleanup_file:
	fclose(f);

	return ret;
}
