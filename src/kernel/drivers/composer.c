#include "composer.h"
#include "video.h"
#include "sched.h"
#include "ipc.h"
#include "shm.h"
#include <stdbool.h>

static wm_window_t windows[MAX_WINDOWS];
static int window_count = 0;
static uint32_t *wallpaper_data = NULL;
static uint32_t mouse_x = 400, mouse_y = 300, mouse_btn = 0, last_mouse_btn = 0;
static int drag_win_idx = -1;
static uint32_t drag_off_x = 0, drag_off_y = 0;
static bool mission_control = false;

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

static void wm_redraw_all();

static void wm_bring_to_front(int index) {
    if (index < 0 || index >= window_count - 1) return;
    wm_window_t tmp = windows[index];
    for (int i = index; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    windows[window_count - 1] = tmp;
}

static void wm_animate_step() {
    bool needed = false;
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active && windows[i].animating) {
            if (windows[i].w < windows[i].target_w) windows[i].w += (windows[i].target_w - windows[i].w) / 4 + 1;
            if (windows[i].h < windows[i].target_h) windows[i].h += (windows[i].target_h - windows[i].h) / 4 + 1;

            if (windows[i].w >= windows[i].target_w && windows[i].h >= windows[i].target_h) {
                windows[i].w = windows[i].target_w;
                windows[i].h = windows[i].target_h;
                windows[i].animating = false;
            }
            needed = true;
        }
    }
    if (needed) wm_redraw_all();
}

static void wm_redraw_all() {
    if (mission_control) {
        video_clear(0x000000);
        // Mostrar ventanas en miniatura (simplificado: rectángulos con el nombre)
        for (int i = 0; i < window_count; i++) {
            if (windows[i].active) {
                int grid_x = 50 + (i % 4) * 180;
                int grid_y = 50 + (i / 4) * 150;
                video_draw_rounded_rect(grid_x, grid_y, 150, 100, 8, 0x88555555);
                video_draw_string("Task Window", grid_x + 10, grid_y + 40, 0xFFFFFF);
            }
        }
        return;
    }

    if (wallpaper_data) {
        // Asumimos que el wallpaper es del tamaño de la pantalla
        // Para simplificar en este entorno, usamos un draw_sprite gigante
        // O mejor una copia directa al FB si es posible.
        // Pero composer no tiene acceso directo al FB struct de video.c de forma limpia
        // Usaremos video_put_pixel por ahora
        for(uint32_t y=0; y<600; y++) {
            for(uint32_t x=0; x<800; x++) {
                video_put_pixel(x, y, wallpaper_data[y * 800 + x]);
            }
        }
    } else {
        video_clear(0x1E1E1E);
    }

    // 1. Dibujar Ventanas
    for (int i = 0; i < window_count; i++) {
        if (windows[i].active) {
            // Sombra
            video_draw_shadow(windows[i].x, windows[i].y, windows[i].w, windows[i].h, 12);
            // Marco de la ventana (Estilo Apple Glass con bordes redondeados)
            video_draw_rounded_rect(windows[i].x, windows[i].y, windows[i].w, windows[i].h, 12, 0x99222222);
            video_draw_rounded_rect(windows[i].x, windows[i].y, windows[i].w, 24, 12, 0xDD444444); // Barra de titulo

            if (windows[i].shm_buffer) {
                for (uint32_t hh=0; hh < windows[i].h - 24; hh++) {
                    for (uint32_t ww=0; ww < windows[i].w; ww++) {
                        video_put_pixel(windows[i].x + ww, windows[i].y + 24 + hh, windows[i].shm_buffer[hh * windows[i].w + ww]);
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
            wm_animate_step();
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
                case 40: // SET_WALLPAPER
                    wallpaper_data = (uint32_t *)shm_at(msg.data[0], 0);
                    wm_redraw_all();
                    break;
                case 41: // TOGGLE_MISSION_CONTROL
                    mission_control = !mission_control;
                    wm_redraw_all();
                    break;
                case COMPOSER_CREATE_WINDOW:
                    if (window_count < MAX_WINDOWS) {
                        windows[window_count].x = (uint32_t)msg.data[0];
                        windows[window_count].y = (uint32_t)msg.data[1];
                        windows[window_count].target_w = (uint32_t)msg.data[2];
                        windows[window_count].target_h = (uint32_t)msg.data[3];
                        // Start small for animation
                        windows[window_count].w = 20;
                        windows[window_count].h = 20;
                        windows[window_count].owner_id = msg.sender;
                        windows[window_count].active = true;
                        windows[window_count].animating = true;
                        window_count++;
                        wm_redraw_all();
                    }
                    break;
                case 20: // MOUSE_EVENT
                    mouse_x = (uint32_t)msg.data[0];
                    mouse_y = (uint32_t)msg.data[1];
                    mouse_btn = (uint32_t)msg.data[2];

                    if (mouse_btn && !last_mouse_btn) {
                        /* Detectar inicio de click o arrastre */
                        drag_win_idx = -1;
                        for (int i = window_count - 1; i >= 0; i--) {
                            if (windows[i].active &&
                                mouse_x >= windows[i].x && mouse_x <= windows[i].x + windows[i].w &&
                                mouse_y >= windows[i].y && mouse_y <= windows[i].y + windows[i].h) {

                                wm_bring_to_front(i);
                                int idx = window_count - 1; // Ahora es la ultima

                                /* Notificar al Shell del cambio de foco (ID 1001) */
                                ipc_msg_t fm;
                                fm.sender = 1;
                                fm.type = 42; // FOCUS_CHANGED
                                fm.data[0] = windows[idx].owner_id;
                                ipc_send(1001, &fm);

                                // ¿Es en la barra de titulo? (24 px segun video.c rounded rect)
                                if (mouse_y >= windows[idx].y && mouse_y <= windows[idx].y + 24) {
                                    drag_win_idx = idx;
                                    drag_off_x = mouse_x - windows[idx].x;
                                    drag_off_y = mouse_y - windows[idx].y;
                                } else {
                                    /* Enviar evento de clic a la aplicación dueña */
                                    ipc_msg_t m;
                                    m.sender = 1;
                                    m.type = 21;  // WM_CLICK
                                    m.data[0] = mouse_x - windows[idx].x;
                                    m.data[1] = mouse_y - windows[idx].y;
                                    ipc_send(windows[idx].owner_id, &m);
                                }
                                break;
                            }
                        }
                    } else if (mouse_btn && drag_win_idx != -1) {
                        /* Arrastrar ventana */
                        windows[drag_win_idx].x = mouse_x - drag_off_x;
                        windows[drag_win_idx].y = mouse_y - drag_off_y;
                    } else if (!mouse_btn) {
                        /* Snapping (Magnet) al soltar */
                        if (drag_win_idx != -1) {
                            if (mouse_x < 50) {
                                // Snap Izquierda
                                windows[drag_win_idx].x = 0; windows[drag_win_idx].y = 0;
                                windows[drag_win_idx].target_w = 400; windows[drag_win_idx].target_h = 600;
                                windows[drag_win_idx].animating = true;
                            } else if (mouse_x > 750) {
                                // Snap Derecha
                                windows[drag_win_idx].x = 400; windows[drag_win_idx].y = 0;
                                windows[drag_win_idx].target_w = 400; windows[drag_win_idx].target_h = 600;
                                windows[drag_win_idx].animating = true;
                            }
                        }
                        drag_win_idx = -1;
                    }

                    last_mouse_btn = mouse_btn;

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
