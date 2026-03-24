#include "composer.h"
#include "video.h"
#include "kernel/sched.h"
#include "kernel/ipc.h"

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
            }
        }
        sched_yield();
    }
}

void composer_start(void) {
    sched_create_task(composer_task, false);
}
