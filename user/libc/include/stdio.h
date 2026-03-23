#ifndef LIB_STDIO_H
#define LIB_STDIO_H

#include <stddef.h>

typedef struct {
    void *vfs_node;
    uint32_t pos;
    int error;
} FILE;

int printf(const char *format, ...);
int putchar(int c);
int puts(const char *s);

/* Nuevas funciones de flujo de archivos */
FILE *fopen(const char *path, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fclose(FILE *stream);

#endif
