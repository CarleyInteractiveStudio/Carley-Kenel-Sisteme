#ifndef SHM_H
#define SHM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_SHM_SEGMENTS 32

typedef struct {
    uint64_t id;
    void *phys_addr;
    size_t size;
    bool used;
} shm_segment_t;

void shm_init(void);
long shm_get(uint64_t id, size_t size);
void *shm_at(uint64_t shm_id, uintptr_t hint);

#endif
