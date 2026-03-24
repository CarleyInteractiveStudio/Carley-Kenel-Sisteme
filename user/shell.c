#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern long syscall0(int num);
extern long syscall1(int num, long arg1);
extern long syscall2(int num, long arg1, long arg2);
extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_READ      4
#define SYS_READDIR   7
#define SYS_EXIT      8
#define SYS_SPAWN     10
#define SYS_OPEN      3
#define SYS_CLOSE     5

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

void shell_ls(void) {
    vfs_dirent_t dirent;
    int index = 0;

    /* Abrir el directorio raiz '/' para obtener su descriptor */
    int fd = (int)syscall1(SYS_OPEN, (long)"/");
    if (fd < 0) {
        printf("Error: No se pudo abrir el directorio raiz.\n");
        return;
    }

    printf("Contenido de / :\n");
    while (syscall3(SYS_READDIR, fd, index, (long)&dirent) == 0) {
        printf("  %s [%s]\n", dirent.name, (dirent.type == 2 ? "DIR" : "FILE"));
        index++;
    }
    syscall1(SYS_CLOSE, fd);
}

void shell_cat(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error: No se pudo abrir %s\n", path);
        return;
    }
    char buf[256];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, 255, f)) > 0) {
        buf[read_bytes] = 0;
        printf("%s", buf);
    }
    printf("\n");
    fclose(f);
}

void main(void) {
    char cmd[64];
    int pos = 0;

    printf("\nCarley Shell v0.3 (Secure Edition)\n");

    for (;;) {
        printf("> ");
        pos = 0;
        memset(cmd, 0, 64);

        while (1) {
            char c;
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
            printf("Comandos: help, ls, cat, gui, exit\n");
        } else if (strcmp(cmd, "ls") == 0) {
            shell_ls();
        } else if (strncmp(cmd, "cat ", 4) == 0) {
            shell_cat(cmd + 4);
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
