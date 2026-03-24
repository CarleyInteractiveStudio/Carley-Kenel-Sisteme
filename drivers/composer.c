#include "composer.h"
#include "video.h"
#include "kernel/sched.h"
#include "kernel/ipc.h"

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
                    /* data[0]=x, data[1]=y, data[2]=w, data[3]=h, data[4]=ptr_data */
                    draw_sprite((uint32_t)msg.data[0], (uint32_t)msg.data[1], (uint32_t)msg.data[2], (uint32_t)msg.data[3], (uint32_t *)msg.data[4]);
                    break;
            }
        }
        sched_yield();
    }
}

void composer_start(void) {
    sched_create_task(composer_task, false);
}
