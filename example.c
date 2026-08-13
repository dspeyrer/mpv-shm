#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

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

	if (fseek(f, 0, SEEK_END) < 0) {
		printf("fseek(): %s\n", strerror(errno));
		goto cleanup_file;
	}

	long len = ftell(f);

	if (len < 0) {
		printf("ftell(): %s\n", strerror(errno));
		goto cleanup_file;
	}

	rewind(f);

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

	size_t n = fread(ptr, len, 1, f);

	if (n == 0) {
		printf("fread(): 0\n");
		goto cleanup_map;
	}

	printf("map available at shm://0+%lx/%s\n", len, argv[2]);
	printf("Press any key to exit...\n");

	getc(stdin);

	ret = 0;
cleanup_map:
	munmap(ptr, len);
cleanup_shm:
	close(shm);
	shm_unlink(shm_name);
cleanup_shm_name:
	free(shm_name);
cleanup_file:
	fclose(f);
	return ret;
}

