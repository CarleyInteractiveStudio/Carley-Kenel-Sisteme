#ifndef LIB_GUI_H
#define LIB_GUI_H

#include <stdint.h>
#include <stddef.h>
#include <ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

// System calls and IPC identifiers
#define COMPOSER_ID 1
#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3
#define COMPOSER_ATTACH_SHM    30
#define WM_CLICK               21

typedef struct {
    int type;
    int x, y;
    int btn;
    int key;
} gui_event_t;

#define GUI_EVENT_CLICK 1
#define GUI_EVENT_KEY   2
#define GUI_EVENT_CLOSE 3

typedef struct {
    uint32_t x, y, w, h;
    uint64_t owner_id;
    uint32_t *buffer;
    long shm_id;
} gui_window_t;

// API functions
gui_window_t *gui_window_create(const char *title, int x, int y, int w, int h);
void gui_window_redraw(gui_window_t *win);
void gui_draw_rect(gui_window_t *win, int x, int y, int w, int h, uint32_t color);
void gui_draw_text(gui_window_t *win, int x, int y, const char *text, uint32_t color);
int gui_poll_event(gui_window_t *win, gui_event_t *event);

#ifdef __cplusplus
}

// C++ Classes for better DX
class CarleyWindow {
public:
    CarleyWindow(const char *title, int x, int y, int w, int h);
    ~CarleyWindow();

    void draw_rect(int x, int y, int w, int h, uint32_t color);
    void draw_text(int x, int y, const char *text, uint32_t color);
    bool poll_event(gui_event_t *event);
    void sync();

    void draw_button(int x, int y, int w, int h, const char *label, uint32_t color);

private:
    gui_window_t *win;
};

#endif

#endif
