#include "composer.h"
#include "video.h"
#include "kernel/sched.h"
#include "kernel/ipc.h"
#include "kernel/shm.h"
#include <stdbool.h>

static wm_window_t windows[MAX_WINDOWS];
static int window_count = 0;
static uint32_t mouse_x = 400, mouse_y = 300, mouse_btn = 0;

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

    // 1. Dibujar Ventanas
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) {
            // Sombra
            video_draw_shadow(windows[i].x, windows[i].y, windows[i].w, windows[i].h, 12);
            // Marco de la ventana (Estilo Apple Glass con bordes redondeados)
            video_draw_rounded_rect(windows[i].x, windows[i].y, windows[i].w, windows[i].h, 12, 0x99222222);
            video_draw_rounded_rect(windows[i].x, windows[i].y, windows[i].w, 24, 12, 0xDD444444); // Barra de titulo

            if (windows[i].shm_buffer) {
                for (uint32_t hh=0; hh < windows[i].h - 20; hh++) {
                    for (uint32_t ww=0; ww < windows[i].w; ww++) {
                        video_put_pixel(windows[i].x + ww, windows[i].y + 20 + hh, windows[i].shm_buffer[hh * windows[i].w + ww]);
                    }
                }
            }
        }
    }

    // 2. Dibujar Cursor (Flecha simple o cruz)
    video_draw_rect(mouse_x, mouse_y, 8, 2, 0xFFFFFF);
    video_draw_rect(mouse_x + 3, mouse_y - 3, 2, 8, 0xFFFFFF);
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
                    /* data[2]=shm_id */
                    for (int i = 0; i < MAX_ICONS; i++) {
                        if (!icons[i].used) {
                            icons[i].w = (uint32_t)msg.data[0];
                            icons[i].h = (uint32_t)msg.data[1];
                            icons[i].data = (uint32_t *)shm_at(msg.data[2], 0);
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
                case 20: // MOUSE_EVENT
                    mouse_x = (uint32_t)msg.data[0];
                    mouse_y = (uint32_t)msg.data[1];
                    mouse_btn = (uint32_t)msg.data[2];

                    if (mouse_btn) {
                        /* Detectar clics en ventanas */
                        for (int i = window_count - 1; i >= 0; i--) {
                            if (windows[i].active &&
                                mouse_x >= windows[i].x && mouse_x <= windows[i].x + windows[i].w &&
                                mouse_y >= windows[i].y && mouse_y <= windows[i].y + windows[i].h) {

                                /* Enviar evento de clic a la aplicación dueña */
                                ipc_msg_t m;
                                m.sender = 1; // From Composer
                                m.type = 21;  // WM_CLICK
                                m.data[0] = mouse_x - windows[i].x;
                                m.data[1] = mouse_y - windows[i].y;
                                ipc_send(windows[i].owner_id, &m);
                                break;
                            }
                        }
                    }

                    wm_redraw_all();
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
