#include <stdint.h>
#include <stddef.h>

extern "C" {
    void __cxa_pure_virtual() { while (1); }
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_finalize(void *) {}

    void *__dso_handle = (void *)&__dso_handle;

    void *malloc(size_t size);
    void free(void *ptr);
}

void *operator new(size_t size) {
    return malloc(size);
}

void *operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void *p) {
    free(p);
}

void operator delete[](void *p) {
    free(p);
}

void operator delete(void *p, size_t sz) {
    (void)sz;
    free(p);
}

void operator delete[](void *p, size_t sz) {
    (void)sz;
    free(p);
}
