#include <gui.h>
#include <ipc.h>

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

long syscall1(int num, long arg1);
long syscall3(int num, long arg1, long arg2, long arg3);
}

#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2

gui_window_t *gui_window_create(const char *title, int x, int y, int w, int h) {
    gui_window_t *win = (gui_window_t *)malloc(sizeof(gui_window_t));
    win->x = x; win->y = y; win->w = w; win->h = h;

    // Generar SHM ID (pseudo-aleatorio para este prototipo o basado en coordenadas)
    win->shm_id = shm_get(2000 + x + y, w * h * 4);
    win->buffer = (uint32_t *)shm_at(win->shm_id, 0);
    memset(win->buffer, 0, w * h * 4);

    // Enviar al Composer
    ipc_msg_t msg;
    msg.sender = 1001; // ID genérico por ahora
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = x; msg.data[1] = y; msg.data[2] = w; msg.data[3] = h;
    syscall3(SYS_IPC_SEND, COMPOSER_ID, (long)&msg, 0);

    // Atar el buffer
    msg.type = 30; // ATTACH_SHM
    msg.data[0] = win->shm_id;
    syscall3(SYS_IPC_SEND, COMPOSER_ID, (long)&msg, 0);

    return win;
}

void gui_draw_rect(gui_window_t *win, int x, int y, int w, int h, uint32_t color) {
    for (int i = y; i < y + h; i++) {
        if (i < 0 || i >= (int)win->h) continue;
        for (int j = x; j < x + w; j++) {
            if (j < 0 || j >= (int)win->w) continue;
            win->buffer[i * win->w + j] = color;
        }
    }
}

void gui_draw_text(gui_window_t *win, int x, int y, const char *text, uint32_t color) {
    (void)win; (void)x; (void)y; (void)text; (void)color;
}

int gui_poll_event(gui_window_t *win, gui_event_t *event) {
    (void)win;
    ipc_msg_t msg;
    if (syscall3(SYS_IPC_RECV, 0, (long)&msg, 0) == 0) {
        if (msg.type == WM_CLICK) {
            event->type = GUI_EVENT_CLICK;
            event->x = (int)msg.data[0];
            event->y = (int)msg.data[1];
            event->btn = (int)msg.data[2];
            return 1;
        }
        if (msg.type == 22) {
            event->type = GUI_EVENT_KEY;
            event->key = (int)msg.data[0];
            return 1;
        }
    }
    return 0;
}

// C++ Implementation
CarleyWindow::CarleyWindow(const char *title, int x, int y, int w, int h) {
    win = gui_window_create(title, x, y, w, h);
}

CarleyWindow::~CarleyWindow() {
    free(win);
}

void CarleyWindow::draw_rect(int x, int y, int w, int h, uint32_t color) {
    gui_draw_rect(win, x, y, w, h, color);
}

void CarleyWindow::draw_text(int x, int y, const char *text, uint32_t color) {
    gui_draw_text(win, x, y, text, color);
}

bool CarleyWindow::poll_event(gui_event_t *event) {
    return gui_poll_event(win, event) != 0;
}

void gui_draw_button(gui_window_t *win, int x, int y, int w, int h, const char *label, uint32_t color) {
    gui_draw_rect(win, x, y, w, h, color);
    gui_draw_rect(win, x + 2, y + 2, w - 4, h - 4, 0x55000000);
    // Para el texto en C usaremos syscall directa por ahora hasta tener motor de texto en libgui
}

void CarleyWindow::draw_button(int x, int y, int w, int h, const char *label, uint32_t color) {
    gui_draw_button(win, x, y, w, h, label, color);
}
