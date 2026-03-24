#ifndef LIB_STDLIB_H
#define LIB_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void exit(int status);

int atoi(const char *nptr);

#endif
