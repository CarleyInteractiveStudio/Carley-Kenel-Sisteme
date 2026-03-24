#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern long syscall0(int num);
extern long syscall1(int num, long arg1);
extern long syscall2(int num, long arg1, long arg2);
extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_READ      4
#define SYS_READDIR   7
#define SYS_EXIT      8
#define SYS_SPAWN     10
#define SYS_OPEN      3
#define SYS_GET_INFO  11

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

void shell_ls(void) {
    vfs_dirent_t dirent;
    int index = 0;
    int fd = (int)syscall1(SYS_OPEN, (long)"/");
    if (fd < 0) return;
    while (syscall3(SYS_READDIR, fd, index, (long)&dirent) == 0) {
        printf("  %s [%s]\n", dirent.name, (dirent.type == 2 ? "DIR" : "FILE"));
        index++;
    }
}

void shell_mem(void) {
    uint64_t total = (uint64_t)syscall1(SYS_GET_INFO, 0);
    uint64_t free = (uint64_t)syscall1(SYS_GET_INFO, 1);
    printf("Memoria Total: %d MB\n", (int)(total / 1024 / 1024));
    printf("Memoria Libre: %d MB\n", (int)(free / 1024 / 1024));
}

void main(void) {
    char cmd[64];
    int pos = 0;

    printf("\nCarley Shell v0.4\n");

    for (;;) {
        printf("> ");
        pos = 0;
        memset(cmd, 0, 64);

        while (1) {
            char c;
            if (syscall3(SYS_READ, 0, (long)&c, 1) > 0) {
                if (c == '\n') { putchar('\n'); break; }
                else if (c == '\b') {
                    if (pos > 0) { pos--; cmd[pos] = 0; putchar('\b'); }
                } else if (pos < 63) { cmd[pos++] = c; putchar(c); }
            }
            syscall0(0);
        }

        if (strcmp(cmd, "help") == 0) {
            printf("Comandos: help, ls, cat, gui, mem, casm, crun, exit\n");
        } else if (strcmp(cmd, "ls") == 0) {
            shell_ls();
        } else if (strcmp(cmd, "mem") == 0) {
            shell_mem();
        } else if (strcmp(cmd, "gui") == 0) {
            syscall1(SYS_SPAWN, (long)"gui.elf");
        } else if (strcmp(cmd, "exit") == 0) {
            syscall1(SYS_EXIT, 0);
        } else if (strlen(cmd) > 0) {
            if (syscall1(SYS_SPAWN, (long)cmd) != 0) {
                printf("Error al lanzar: %s\n", cmd);
            }
        }
    }
}
