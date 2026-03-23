#include <stdarg.h>
#include "include/stdio.h"
#include "include/string.h"
#include "include/stdlib.h"

extern long syscall1(int num, long arg1);
extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_OPEN  3
#define SYS_READ  4
#define SYS_WRITE 6

int putchar(int c) {
    char buf = (char)c;
    syscall3(SYS_WRITE, 1, (long)&buf, 1);
    return c;
}

int puts(const char *s) {
    size_t len = strlen(s);
    syscall3(SYS_WRITE, 1, (long)s, len);
    putchar('\n');
    return 0;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = 0;
    while (*format) {
        if (*format == '%' && *(format + 1) == 's') {
            const char *s = va_arg(args, const char *);
            size_t len = strlen(s);
            syscall3(SYS_WRITE, 1, (long)s, len);
            format += 2;
        } else {
            putchar(*format++);
            count++;
        }
    }
    va_end(args);
    return count;
}

FILE *fopen(const char *path, const char *mode) {
    (void)mode;
    void *node = (void *)syscall1(SYS_OPEN, (long)path);
    if (!node) return NULL;

    FILE *f = malloc(sizeof(FILE));
    f->vfs_node = node;
    f->pos = 0;
    f->error = 0;
    return f;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream) return 0;
    long ret = syscall3(SYS_READ, (long)stream->vfs_node, (long)ptr, size * nmemb);
    if (ret < 0) { stream->error = 1; return 0; }
    return (size_t)ret / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream) return 0;
    long ret = syscall3(SYS_WRITE, (long)stream->vfs_node, (long)ptr, size * nmemb);
    if (ret < 0) { stream->error = 1; return 0; }
    return (size_t)ret / size;
}

int fclose(FILE *stream) {
    if (stream) free(stream);
    return 0;
}
