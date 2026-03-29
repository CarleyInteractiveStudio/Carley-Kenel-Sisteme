#include "stdio.h"
#include "string.h"
#include <stdarg.h>

static void sprintf_uint(char **str, uint64_t n, int base) {
    char buf[32];
    int i = 0;
    const char *digits = "0123456789ABCDEF";
    if (n == 0) { *(*str)++ = '0'; return; }
    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }
    while (--i >= 0) *(*str)++ = buf[i];
}

static void sprintf_int(char **str, int64_t n) {
    if (n < 0) { *(*str)++ = '-'; n = -n; }
    sprintf_uint(str, (uint64_t)n, 10);
}

int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *p = str;
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (!s) s = "(null)";
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'd':
                case 'i':
                    sprintf_int(&p, va_arg(args, int));
                    break;
                case 'u':
                    sprintf_uint(&p, va_arg(args, unsigned int), 10);
                    break;
                case 'x':
                case 'X':
                    sprintf_uint(&p, va_arg(args, unsigned int), 16);
                    break;
                case 'c':
                    *p++ = (char)va_arg(args, int);
                    break;
                case '%':
                    *p++ = '%';
                    break;
            }
            format++;
        } else {
            *p++ = *format++;
        }
    }
    *p = 0;
    va_end(args);
    return (int)(p - str);
}
