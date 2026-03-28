#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#include <stdio.h>
#include <string.h>

#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3
#define COMPOSER_CLEAR_WINDOW  4 // I'll assume 4 is clear or just draw a rect

#define SYS_YIELD     0
#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2
#define SYS_OPEN      3
#define SYS_READDIR   7
#define SYS_SPAWN     10

extern "C" long syscall0(int num);
extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

char current_path[256] = "/";
vfs_dirent_t files[20];
int file_count = 0;

void draw_explorer() {
    ipc_msg_t msg;
    msg.sender = 2001;

    // Clear window area (background)
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 50; msg.data[1] = 50+20; msg.data[2] = 400; msg.data[3] = 280;
    msg.data[4] = 0x2C3E50;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    // Draw Header / Path
    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[1] = 60; msg.data[2] = 55; msg.data[3] = 0xFFFFFF;
    char header[300];
    sprintf(header, "Path: %s", current_path);
    for(int i=0; header[i]; i++) {
        msg.data[0] = header[i];
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
        msg.data[1] += 8;
    }

    // List files
    long fd = syscall1(SYS_OPEN, (long)current_path);
    file_count = 0;

    // Entry for ".." (back)
    if (strcmp(current_path, "/") != 0) {
        strcpy(files[0].name, "..");
        files[0].type = 2; // DIR
        file_count = 1;
    }

    if (fd >= 0) {
        int y_offset = 100;
        int readdir_idx = 0;
        while (syscall3(SYS_READDIR, fd, readdir_idx++, (long)&files[file_count]) == 0) {
            // Skip "." and ".." if the filesystem provides them, as we handle them manually
            if (strcmp(files[file_count].name, ".") == 0 || strcmp(files[file_count].name, "..") == 0) continue;

            // Icon
            msg.type = COMPOSER_DRAW_RECT;
            msg.data[0] = 60; msg.data[1] = y_offset - 5; msg.data[2] = 12; msg.data[3] = 12;
            msg.data[4] = (files[file_count].type == 2) ? 0x3498DB : 0xBDC3C7;
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

            // Name
            msg.type = COMPOSER_DRAW_CHAR;
            msg.data[1] = 80; msg.data[2] = y_offset; msg.data[3] = 0xFFFFFF;
            for(int i=0; files[file_count].name[i]; i++) {
                msg.data[0] = files[file_count].name[i];
                syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
                msg.data[1] += 8;
            }

            y_offset += 20;
            file_count++;
            if (file_count >= 20 || y_offset > 320) break;
        }
    }
}

int main() {
    printf("Carley Explorer: Iniciando...\n");

    // Crear ventana
    ipc_msg_t msg;
    msg.sender = 2001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 50; msg.data[1] = 50; msg.data[2] = 400; msg.data[3] = 300;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    draw_explorer();

    while(1) {
        ipc_msg_t rmsg;
        if (syscall3(SYS_IPC_RECV, 0, (long)&rmsg, 0) == 0) {
            if (rmsg.type == 21) { // WM_CLICK
                int click_x = rmsg.data[0];
                int click_y = rmsg.data[1];

                // Check if click was on a file/folder
                if (click_y >= 50 && click_y <= 300) {
                    int file_index = (click_y - 100 + 5) / 20;
                    if (file_index >= 0 && file_index < file_count) {
                        printf("Explorer: Click en %s\n", files[file_index].name);

                        if (files[file_index].type == 2) { // Directory
                    if (strcmp(files[file_index].name, "..") == 0) {
                        // Go back: Find last '/' and truncate
                        char *last_slash = strrchr(current_path, '/');
                        if (last_slash == current_path) {
                            strcpy(current_path, "/");
                        } else if (last_slash) {
                            *last_slash = 0;
                        }
                            } else {
                        if (strcmp(current_path, "/") == 0) {
                            sprintf(current_path, "/%s", files[file_index].name);
                        } else {
                            strcat(current_path, "/");
                            strcat(current_path, files[file_index].name);
                        }
                            }
                            draw_explorer();
                        } else { // File
                            char full_path[512];
                            if (strcmp(current_path, "/") == 0) {
                                sprintf(full_path, "%s", files[file_index].name);
                            } else {
                                sprintf(full_path, "%s/%s", current_path, files[file_index].name);
                            }

                            if (strstr(full_path, ".elf")) {
                                printf("Explorer: Lanzando %s\n", full_path);
                                syscall1(SYS_SPAWN, (long)full_path);
                            }
                        }
                    }
                }
            }
        }
        syscall0(SYS_YIELD);
    }
    return 0;
}
