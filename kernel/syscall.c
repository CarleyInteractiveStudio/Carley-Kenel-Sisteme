#include "syscall.h"
#include "ipc.h"
#include "sched.h"

/* Manejador común de syscalls */
context_t *syscall_handler(context_t *ctx) {
    /* Guardamos el número de syscall en rax */
    uint64_t sys_no = ctx->rax;

    switch (sys_no) {
        case SYS_YIELD:
            /* Forzamos el cambio de tarea inmediatamente */
            return sched_schedule(ctx);

        case SYS_IPC_SEND:
            /* RDI = dest_id, RSI = msg_ptr */
            ctx->rax = ipc_send(ctx->rdi, (ipc_msg_t *)ctx->rsi);
            break;

        case SYS_IPC_RECV:
            /* RDI = msg_ptr */
            ctx->rax = ipc_recv((ipc_msg_t *)ctx->rdi);
            break;

        default:
            ctx->rax = -1;
            break;
    }

    return ctx;
}

void syscall_init(void) {
    /* El mecanismo de inicialización está integrado en idt_init() a través de int $0x80 */
}
