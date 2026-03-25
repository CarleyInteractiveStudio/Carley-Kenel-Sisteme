#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#include <stddef.h>
#include <stdint.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
    void *vfs_node;
    uint32_t pos;
    int error;
} FILE;

int printf(const char *format, ...);
int putchar(int c);
int puts(const char *s);

FILE *fopen(const char *path, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fclose(FILE *stream);

#endif
