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

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

void shell_ls(void) {
    vfs_dirent_t dirent;
    int index = 0;
    printf("Contenido de / :\n");
    while (syscall3(SYS_READDIR, 0, index, (long)&dirent) == 0) {
        printf("  %s\n", dirent.name);
        index++;
    }
}

void shell_cat(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error: No se pudo abrir %s\n", path);
        return;
    }
    char buf[256];
    size_t read;
    while ((read = fread(buf, 1, 255, f)) > 0) {
        buf[read] = 0;
        printf("%s", buf);
    }
    printf("\n");
    fclose(f);
}

void main(void) {
    char cmd[64];
    int pos = 0;

    printf("\nCarley Shell v0.3\n");
    printf("Comandos: help, ls, cat [file], clear, exit, [programa]\n");

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
            printf("Comandos: help, ls, cat, clear, exit\n");
        } else if (strcmp(cmd, "ls") == 0) {
            shell_ls();
        } else if (strncmp(cmd, "cat ", 4) == 0) {
            shell_cat(cmd + 4);
        } else if (strcmp(cmd, "clear") == 0) {
            /* video_clear no está expuesto vía syscall aún, simulamos */
            for(int i=0; i<30; i++) putchar('\n');
        } else if (strcmp(cmd, "exit") == 0) {
            syscall1(SYS_EXIT, 0);
        } else if (strlen(cmd) > 0) {
            if (syscall1(SYS_SPAWN, (long)cmd) != 0) {
                printf("Comando no encontrado: %s\n", cmd);
            }
        }
    }
}
