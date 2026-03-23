#include <stdarg.h>
#include "include/stdio.h"
#include "include/string.h"

/* Syscall wrappers (definidos en syscall.s) */
extern long syscall1(int num, long arg1);
extern long syscall3(int num, long arg1, long arg2, long arg3);

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
            syscall3(SYS_WRITE, 1, (long)s, len); // Imprimir sin nueva línea extra
            format += 2;
        } else {
            putchar(*format++);
            count++;
        }
    }

    va_end(args);
    return count;
}
