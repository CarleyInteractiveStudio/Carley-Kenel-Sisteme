#include "include/stdio.h"
#include "include/string.h"
#include <stdint.h>

/* Syscall wrappers */
extern long syscall0(int num);
extern long syscall1(int num, long arg1);
extern long syscall2(int num, long arg1, long arg2);
extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_READ      4
#define SYS_READDIR   7
#define SYS_EXIT      8

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

void shell_ls(void) {
    vfs_dirent_t dirent;
    int index = 0;
    while (syscall3(SYS_READDIR, 0, index, (long)&dirent) == 0) {
        printf("  %s\n", dirent.name);
        index++;
    }
}

void main(void) {
    char cmd[64];
    int pos = 0;

    printf("\nCarley Shell v0.1\n");
    printf("Escribe 'help' para ver los comandos.\n");

    for (;;) {
        printf("> ");
        pos = 0;
        memset(cmd, 0, 64);

        while (1) {
            char c;
            /* fd=0 (stdin), buffer=&c, size=1 */
            if (syscall3(SYS_READ, 0, (long)&c, 1) > 0) {
                if (c == '\n') {
                    putchar('\n');
                    break;
                } else if (c == '\b') {
                    if (pos > 0) {
                        pos--;
                        cmd[pos] = 0;
                        putchar('\b');
                    }
                } else if (pos < 63) {
                    cmd[pos++] = c;
                    putchar(c);
                }
            }
            syscall0(0);
        }

        if (strcmp(cmd, "help") == 0) {
            printf("Comandos: help, ls, clear, exit\n");
        } else if (strcmp(cmd, "ls") == 0) {
            shell_ls();
        } else if (strcmp(cmd, "clear") == 0) {
            printf("(Pantalla Limpiada)\n");
        } else if (strcmp(cmd, "exit") == 0) {
            syscall1(SYS_EXIT, 0);
        } else if (strlen(cmd) > 0) {
            printf("Comando no reconocido: %s\n", cmd);
        }
    }
}
