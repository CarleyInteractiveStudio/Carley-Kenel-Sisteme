#include "syscall.h"
#include "ipc.h"
#include "sched.h"
#include "vfs.h"
#include "drivers/video.h"
#include "keyboard_buf.h"
#include "pmm.h"
#include "vmm.h"
#include "elf.h"

context_t *syscall_handler(context_t *ctx) {
    uint64_t sys_no = ctx->rax;
    task_t *curr = sched_get_current_task();

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
            if (ctx->rdi == 0) {
                ctx->rax = kbd_buf_read((char *)ctx->rsi, (size_t)ctx->rdx);
            } else {
                ctx->rax = vfs_read((vfs_node_t *)ctx->rdi, 0, (uint32_t)ctx->rdx, (uint8_t *)ctx->rsi);
            }
            break;

        case SYS_WRITE:
            if (ctx->rdi == 1) { // stdout
                char *buf = (char *)ctx->rsi;
                for (size_t i = 0; i < ctx->rdx; i++) {
                    /* Usar la nueva función con soporte de scroll */
                    video_terminal_write(buf[i], 0xFFFFFF);
                }
                ctx->rax = ctx->rdx;
            } else {
                ctx->rax = vfs_write((vfs_node_t *)ctx->rdi, 0, (uint32_t)ctx->rdx, (uint8_t *)ctx->rsi);
            }
            break;

        case SYS_READDIR:
            ctx->rax = vfs_readdir((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (vfs_dirent_t *)ctx->rdx);
            break;

        case SYS_EXIT:
            sched_terminate_task();
            return sched_schedule(ctx);

        case SYS_SBRK: {
            uintptr_t old_heap = curr->heap_end;
            size_t size = (size_t)ctx->rdi;
            if (size > 0) {
                size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
                for (size_t i = 0; i < pages; i++) {
                    void *phys = pmm_alloc_page();
                    vmm_map(curr->pml4, curr->heap_end, (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
                    curr->heap_end += PAGE_SIZE;
                }
            }
            ctx->rax = old_heap;
            break;
        }

        case SYS_SPAWN:
            ctx->rax = elf_load((const char *)ctx->rdi);
            break;

        default:
            ctx->rax = -1;
            break;
    }

    return ctx;
}

void syscall_init(void) {
}
