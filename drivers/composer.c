#include "composer.h"
#include "video.h"
#include "kernel/sched.h"
#include "kernel/ipc.h"
#include "kernel/shm.h"
#include <stdbool.h>

static wm_window_t windows[MAX_WINDOWS];
static int window_count = 0;

#define MAX_ICONS 32
typedef struct {
    uint32_t w, h;
    uint32_t *data;
    bool used;
} composer_icon_t;

static composer_icon_t icons[MAX_ICONS];

static void draw_sprite(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t *data) {
    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            uint32_t color = data[i * w + j];
            /* 0xFF00FF es nuestro magenta transparente de demo */
            if (color != 0xFF00FF) {
                video_put_pixel(x + j, y + i, color);
            }
        }
    }
}

static void wm_redraw_all() {
    video_clear(0x1E1E1E);
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) {
            // Marco de la ventana (Estilo Apple Glass)
            video_draw_rect(windows[i].x, windows[i].y, windows[i].w, windows[i].h, 0x88333333);
            video_draw_rect(windows[i].x, windows[i].y, windows[i].w, 20, 0xCC555555); // Barra de titulo

            if (windows[i].shm_buffer) {
                for (uint32_t hh=0; hh < windows[i].h - 20; hh++) {
                    for (uint32_t ww=0; ww < windows[i].w; ww++) {
                        video_put_pixel(windows[i].x + ww, windows[i].y + 20 + hh, windows[i].shm_buffer[hh * windows[i].w + ww]);
                    }
                }
            }
        }
    }
}

void composer_task(void) {
    ipc_msg_t msg;

    for (;;) {
        if (ipc_recv(&msg) == 0) {
            switch (msg.type) {
                case COMPOSER_DRAW_PIXEL:
                    video_put_pixel((uint32_t)msg.data[0], (uint32_t)msg.data[1], (uint32_t)msg.data[2]);
                    break;
                case COMPOSER_DRAW_RECT:
                    video_draw_rect((uint32_t)msg.data[0], (uint32_t)msg.data[1], (uint32_t)msg.data[2], (uint32_t)msg.data[3], (uint32_t)msg.data[4]);
                    break;
                case COMPOSER_DRAW_CHAR:
                    video_draw_char((char)msg.data[0], (uint32_t)msg.data[1], (uint32_t)msg.data[2], (uint32_t)msg.data[3]);
                    break;
                case COMPOSER_CLEAR:
                    video_clear((uint32_t)msg.data[0]);
                    break;
                case COMPOSER_DRAW_SPRITE:
                    draw_sprite((uint32_t)msg.data[0], (uint32_t)msg.data[1], (uint32_t)msg.data[2], (uint32_t)msg.data[3], (uint32_t *)msg.data[4]);
                    break;
                case COMPOSER_LOAD_ICON: {
                    for (int i = 0; i < MAX_ICONS; i++) {
                        if (!icons[i].used) {
                            icons[i].w = (uint32_t)msg.data[0];
                            icons[i].h = (uint32_t)msg.data[1];
                            icons[i].data = (uint32_t *)msg.data[2];
                            icons[i].used = true;
                            break;
                        }
                    }
                    break;
                }
                case COMPOSER_CREATE_WINDOW:
                    if (window_count < MAX_WINDOWS) {
                        windows[window_count].x = (uint32_t)msg.data[0];
                        windows[window_count].y = (uint32_t)msg.data[1];
                        windows[window_count].w = (uint32_t)msg.data[2];
                        windows[window_count].h = (uint32_t)msg.data[3];
                        windows[window_count].owner_id = msg.sender;
                        windows[window_count].active = true;
                        window_count++;
                        wm_redraw_all();
                    }
                    break;
                case COMPOSER_ATTACH_SHM: {
                    for (int i = 0; i < window_count; i++) {
                        if (windows[i].owner_id == msg.sender) {
                            windows[i].shm_buffer = (uint32_t *)shm_at(msg.data[0], 0);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        sched_yield();
    }
}

void composer_start(void) {
    task_t *t = sched_create_task(composer_task, false);
    t->id = 1; // Reservado para Graphics Server
}
