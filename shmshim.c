#include "shmshim.h"

#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

#else

#include <sys/mman.h>
#include <unistd.h>

#endif

struct shm_shim_shm {
	uint8_t *ptr;
#ifdef _WIN32
	HANDLE handle;	
#else
	char *name;
	size_t len;
	int fd;
#endif
};

uint8_t **shm_shim_create(const uint8_t *name, size_t name_len, size_t shm_len) {
	struct shm_shim_shm *shm = malloc(sizeof(struct shm_shim_shm));

#ifdef _WIN32

	shm->ptr = NULL;
	shm->handle = NULL;

	int wname_len = 1 + MultiByteToWideChar(CP_UTF8, 0, name, name_len, NULL, 0);
	char *wname = malloc(wname_len);
	MultiByteToWideChar(CP_UTF8, 0, name, name_len, wname, wname_len);
	wname[wname_len] = 0;
	
	shm->handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, shm_len << 32, shm_len, wname);

	if (shm->handle == NULL) {
		goto failure;
	}

	shm->ptr = MapViewOfFile(shm->handle, FILE_MAP_ALL_ACCESS, 0, 0, shm_len);

	if (shm->ptr == NULL) {
		goto failure;
	}

#else

	shm->ptr = MAP_FAILED;
	shm->fd = -1;
	shm->len = shm_len;

	shm->name = malloc(name_len + 2);
	shm->name[0] = '/';
	memcpy(shm->name + 1, name, name_len);
	shm->name[name_len + 2] = 0;

	shm_unlink(shm->name);
	shm->fd = shm_open(shm->name, O_RDWR | O_CREAT, 00660);

	if (shm->fd < 0) {
		goto failure;
	}

	int res = ftruncate(shm->fd, shm_len);

	if (res < 0) {
		goto failure;
	}

	shm->ptr = mmap(NULL, shm_len, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);

	if (shm->ptr == MAP_FAILED) {
		goto failure;
	}

#endif

	return &shm->ptr;

failure:
	shm_shim_destroy(&shm->ptr);
	return NULL;
}

void shm_shim_destroy(uint8_t **map) {
	struct shm_shim_shm *shm = (struct shm_shim_shm *)map;

#ifdef _WIN32

	if (shm->ptr != NULL) {
		UnmapViewOfFile(ptr);
	}

	if (shm->handle != NULL) {
		CloseHandle(shm);
	}

#else
	if (shm->ptr != MAP_FAILED) {
		munmap(shm->ptr, shm->len);
	}

	if (shm->fd >= 0) {
		close(shm->fd);
		shm_unlink(shm->name);
	}

	free(shm->name);
#endif

	free(shm);
}
