#include "syscall.h"
#include "ipc.h"
#include "sched.h"
#include "vfs.h"
#include "drivers/video.h"

/* Coordenadas globales para stdout (demo) */
static uint32_t term_x = 10;
static uint32_t term_y = 150;

context_t *syscall_handler(context_t *ctx) {
    uint64_t sys_no = ctx->rax;

    switch (sys_no) {
        case SYS_YIELD:
            return sched_schedule(ctx);

        case SYS_IPC_SEND:
            ctx->rax = ipc_send(ctx->rdi, (ipc_msg_t *)ctx->rsi);
            break;

        case SYS_IPC_RECV:
            ctx->rax = ipc_recv((ipc_msg_t *)ctx->rdi);
            break;

        case SYS_OPEN:
            ctx->rax = (uintptr_t)vfs_open((const char *)ctx->rdi);
            break;

        case SYS_READ:
            ctx->rax = vfs_read((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (uint32_t)ctx->rdx, (uint8_t *)ctx->r10);
            break;

        case SYS_WRITE:
            if (ctx->rdi == 1) { // stdout
                char *buf = (char *)ctx->rsi;
                for (size_t i = 0; i < ctx->rdx; i++) {
                    if (buf[i] == '\n') {
                        term_x = 10;
                        term_y += 10;
                    } else {
                        video_draw_char(buf[i], term_x, term_y, 0xFFFFFF);
                        term_x += 8;
                    }
                    if (term_x > 600) { term_x = 10; term_y += 10; }
                }
                ctx->rax = ctx->rdx;
            }
            break;

        default:
            ctx->rax = -1;
            break;
    }

    return ctx;
}

void syscall_init(void) {
}
