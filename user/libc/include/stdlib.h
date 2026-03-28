#ifndef LIB_STDLIB_H
#define LIB_STDLIB_H

#include <stddef.h>
#include <stdint.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void exit(int status);

int atoi(const char *nptr);

int mkdir(const char *path);
int mkfile(const char *path, uint32_t size);

long shm_get(uint64_t id, size_t size);
void *shm_at(long shm_id, void *addr);

#endif
