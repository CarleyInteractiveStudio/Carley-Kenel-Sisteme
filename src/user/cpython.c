#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gui.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);

/* Carley Script v0.2 - Modern UI Support */

gui_window_t *current_win = NULL;

void execute_line(char *line) {
    if (strncmp(line, "print ", 6) == 0) {
        printf("%s\n", line + 6);
    } else if (strncmp(line, "calc ", 5) == 0) {
        int a = atoi(line + 5);
        printf("Resultado: %d\n", a);
    } else if (strncmp(line, "touch ", 6) == 0) {
        if (mkfile(line + 6, 1024) == 0) printf("OK.\n");
        else printf("Error.\n");
    } else if (strncmp(line, "window.create(", 14) == 0) {
        current_win = gui_window_create("Python App", 100, 100, 400, 300);
        printf("Ventana creada.\n");
    } else if (strncmp(line, "window.draw_rect(", 17) == 0) {
        if (current_win) {
            // Simplificado: window.draw_rect(color)
            uint32_t color = 0x3498DB;
            gui_draw_rect(current_win, 50, 50, 100, 100, color);
            printf("Rectangulo dibujado.\n");
        }
    } else if (strcmp(line, "help") == 0) {
        printf("Comandos: print, calc, touch, window.create(), window.draw_rect(), exit\n");
    }
}

void main(int argc, char **argv) {
    char buf[128];
    printf("Carley Script v0.1 (Python-like) - Escribe 'help' para ayuda.\n");

    if (argc > 1) {
        // Modo script: leer archivo
        FILE *f = fopen(argv[1], "r");
        if (f) {
            while (fgets(buf, 128, f)) {
                execute_line(buf);
            }
            fclose(f);
        }
        return;
    }

    // Modo REPL
    for (;;) {
        printf(">>> ");
        // Usar read(0, ...) manual por ahora
        int len = syscall3(4, 0, (long)buf, 127);
        if (len > 0) {
            buf[len-1] = 0; // Quitar newline
            if (strcmp(buf, "exit") == 0) break;
            execute_line(buf);
        }
    }
}
