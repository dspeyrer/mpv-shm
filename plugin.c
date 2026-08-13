#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <mpv/client.h>
#include <mpv/stream_cb.h>

struct shm_state {
	// The underlying shared memory object
	int shm;
	// A pointer to the data
	char *ptr;
	// The current offset into the data
	size_t off;
	// The length of the data
	size_t len;
};

static int64_t read_cb(void *cookie, char *buf, uint64_t nbytes) {
	struct shm_state *st = cookie;

	uint64_t max = st->len - st->off;

	if (nbytes > max) {
		nbytes = max;
	}

	memcpy(buf, st->ptr + st->off, nbytes);

	st->off += nbytes;

	return nbytes;
}

static int64_t seek_cb(void *cookie, int64_t offset) {
	struct shm_state *st = cookie;

	if (offset < 0) {
		printf("mpv-shm: ERROR: seek got negative offset "PRIi64"\n", offset);
		return MPV_ERROR_GENERIC;
	}

	if (offset > st->len) {
		offset = st->len;
	}

	return (st->off = offset);
}

static int64_t size_cb(void *cookie) {
	return ((struct shm_state *) cookie)->len;
}

static void close_cb(void *cookie) {
	struct shm_state *st = cookie;

	munmap(st->ptr, st->len);
	close(st->shm);

	free(st);
}

static int open_cb(void *_udata, char *uri, mpv_stream_cb_info *info) {
	(void) _udata;

	uint64_t off = 0;
	uint64_t len = 0;
	size_t parse_off = 0;

	sscanf(uri, "shm://%"SCNx64"+%"SCNx64"/%n", &off, &len, &parse_off);

	if (parse_off == 0) {
		printf("mpv-shm - invalid uri %s\n", uri);
		return MPV_ERROR_LOADING_FAILED;
	}

	const char *shm_name = uri + parse_off - 1;
	
	int shm = shm_open(shm_name, O_RDONLY);

	if (shm < 0) {
		printf("mpv-shm - shm_open(): %s\n", strerror(errno));
		return MPV_ERROR_LOADING_FAILED;
	}

	char *ptr = mmap(NULL, len, PROT_READ, MAP_SHARED, shm, off);

	if (ptr == MAP_FAILED) {
		printf("mpv-shm - mmap(): %s\n", strerror(errno));
		return MPV_ERROR_LOADING_FAILED;
	}

	struct shm_state *st = malloc(sizeof(struct shm_state));

	st->shm = shm;
	st->ptr = ptr;
	st->off = 0;
	st->len = len;

	info->cookie = st;

	info->read_fn = read_cb;
	info->seek_fn = seek_cb;
	info->size_fn = size_cb;
	info->close_fn = close_cb;

	return 0;
}

int mpv_open_cplugin(mpv_handle *handle) {
	int res = mpv_stream_cb_add_ro(handle, "shm", NULL, open_cb);

	if (res >= 0) {
		while (1) {
			mpv_event *event = mpv_wait_event(handle, -1);

			if (event->event_id == MPV_EVENT_SHUTDOWN) {
				break;
			}
		}

		return 0;
	} else {
		printf("mpv-shm - mpv_stream_cb_add_ro(): %s\n", mpv_error_string(res));
		return -1;
	}
}
