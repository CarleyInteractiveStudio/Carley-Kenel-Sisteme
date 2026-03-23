#include "syscall.h"
#include "ipc.h"
#include "sched.h"
#include "vfs.h"

/* Manejador común de syscalls */
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
            /* RDI = const char *path */
            ctx->rax = (uintptr_t)vfs_open((const char *)ctx->rdi);
            break;

        case SYS_READ:
            /* RDI = vfs_node_t*, RSI = offset, RDX = size, R10 = buffer */
            ctx->rax = vfs_read((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (uint32_t)ctx->rdx, (uint8_t *)ctx->r10);
            break;

        default:
            ctx->rax = -1;
            break;
    }

    return ctx;
}

void syscall_init(void) {
}
