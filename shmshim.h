#include <stdint.h>
#include <stdlib.h>

uint8_t **shm_shim_create(const uint8_t *name, size_t name_len, size_t shm_len);
void shm_shim_destroy(uint8_t **map);
