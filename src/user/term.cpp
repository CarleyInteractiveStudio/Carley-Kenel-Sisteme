#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3

#define SYS_READ      4
#define SYS_WRITE     6
#define SYS_YIELD     0
#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2

extern "C" long syscall0(int num);
extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

#define TERM_W 640
#define TERM_H 400
#define ROWS 20
#define COLS 70

char buffer[ROWS][COLS];
int cursor_x = 0, cursor_y = 0;

void draw_terminal() {
    ipc_msg_t msg;
    msg.sender = 6001;

    // Background
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 100; msg.data[1] = 100+20; msg.data[2] = TERM_W; msg.data[3] = TERM_H-20;
    msg.data[4] = 0x000000;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0x00FF00; // Green text
    for (int y = 0; y < ROWS; y++) {
        msg.data[1] = 110; msg.data[2] = 130 + (y * 18);
        for (int x = 0; x < COLS; x++) {
            if (buffer[y][x]) {
                msg.data[0] = buffer[y][x];
                syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
                msg.data[1] += 9;
            }
        }
    }
}

void term_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        buffer[cursor_y][cursor_x] = ' ';
    } else {
        buffer[cursor_y][cursor_x++] = c;
        if (cursor_x >= COLS) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= ROWS) {
        // Scroll
        for (int i = 0; i < ROWS - 1; i++) memcpy(buffer[i], buffer[i+1], COLS);
        memset(buffer[ROWS-1], 0, COLS);
        cursor_y = ROWS - 1;
    }
}

int main() {
    memset(buffer, 0, sizeof(buffer));

    // Crear ventana
    ipc_msg_t msg;
    msg.sender = 6001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 100; msg.data[1] = 100; msg.data[2] = TERM_W; msg.data[3] = TERM_H;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    const char *welcome = "Carley Terminal v1.0\nType 'help' for commands\n\n> ";
    for (int i = 0; welcome[i]; i++) term_putc(welcome[i]);
    draw_terminal();

    char cmd[COLS];
    int cmd_pos = 0;

    while(1) {
        char c;
        if (syscall3(SYS_READ, 0, (long)&c, 1) > 0) {
            if (c == '\n') {
                term_putc('\n');
                cmd[cmd_pos] = 0;

                if (strcmp(cmd, "help") == 0) {
                    const char *h = "Available: ls, about, clear, exit\n";
                    for(int i=0; h[i]; i++) term_putc(h[i]);
                } else if (strcmp(cmd, "clear") == 0) {
                    memset(buffer, 0, sizeof(buffer));
                    cursor_x = 0; cursor_y = 0;
                } else if (strcmp(cmd, "about") == 0) {
                    syscall1(10, (long)"about.elf"); // SYS_SPAWN
                } else if (strcmp(cmd, "ls") == 0) {
                    // Simular ls para no complicar el pipe por ahora
                    const char *l = "bin/ dev/ disk/ ram/\n";
                    for(int i=0; l[i]; i++) term_putc(l[i]);
                } else if (strlen(cmd) > 0) {
                    const char *e = "Command not found.\n";
                    for(int i=0; e[i]; i++) term_putc(e[i]);
                }

                const char *p = "> ";
                for(int i=0; p[i]; i++) term_putc(p[i]);
                cmd_pos = 0;
            } else if (c == '\b') {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    term_putc('\b');
                }
            } else if (cmd_pos < COLS - 1) {
                cmd[cmd_pos++] = c;
                term_putc(c);
            }
            draw_terminal();
        }
        syscall0(SYS_YIELD);
    }

    return 0;
}
