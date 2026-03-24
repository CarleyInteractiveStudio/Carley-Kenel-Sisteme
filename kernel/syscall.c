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
            ctx->rax = ipc_send(ctx->rdi, (void *)ctx->rsi);
            break;

        case SYS_IPC_RECV:
            ctx->rax = ipc_recv((void *)ctx->rdi);
            break;

        case SYS_OPEN: {
            /* RDI = const char *path. Retorna FD o -1 */
            vfs_node_t *node = vfs_open((const char *)ctx->rdi);
            if (!node) { ctx->rax = -1; break; }

            /* Buscar slot libre en la tabla de archivos del proceso */
            int fd = -1;
            for (int i = 0; i < MAX_FILES_PER_TASK; i++) {
                if (curr->files[i] == NULL) {
                    curr->files[i] = node;
                    fd = i;
                    break;
                }
            }
            ctx->rax = fd;
            break;
        }

        case SYS_READ: {
            /* RDI = fd, RSI = buffer, RDX = size */
            int fd = (int)ctx->rdi;
            if (fd == 0) { // stdin
                ctx->rax = kbd_buf_read((char *)ctx->rsi, (size_t)ctx->rdx);
            } else if (fd > 0 && fd < MAX_FILES_PER_TASK && curr->files[fd]) {
                ctx->rax = vfs_read(curr->files[fd], 0, (uint32_t)ctx->rdx, (uint8_t *)ctx->rsi);
            } else {
                ctx->rax = -1;
            }
            break;
        }

        case SYS_WRITE: {
            /* RDI = fd, RSI = buffer, RDX = size */
            int fd = (int)ctx->rdi;
            if (fd == 1) { // stdout
                char *buf = (char *)ctx->rsi;
                for (size_t i = 0; i < ctx->rdx; i++) {
                    video_terminal_write(buf[i], 0xFFFFFF);
                }
                ctx->rax = ctx->rdx;
            } else if (fd > 1 && fd < MAX_FILES_PER_TASK && curr->files[fd]) {
                ctx->rax = vfs_write(curr->files[fd], 0, (uint32_t)ctx->rdx, (uint8_t *)ctx->rsi);
            } else {
                ctx->rax = -1;
            }
            break;
        }

        case SYS_READDIR: {
            /* RDI = fd (del directorio), RSI = index, RDX = vfs_dirent_t* */
            int fd = (int)ctx->rdi;
            if (fd >= 0 && fd < MAX_FILES_PER_TASK && curr->files[fd]) {
                ctx->rax = vfs_readdir(curr->files[fd], (uint32_t)ctx->rsi, (vfs_dirent_t *)ctx->rdx);
            } else {
                ctx->rax = -1;
            }
            break;
        }

        case SYS_CLOSE: {
            int fd = (int)ctx->rdi;
            if (fd >= 0 && fd < MAX_FILES_PER_TASK && curr->files[fd]) {
                curr->files[fd] = NULL; // En un sistema real, liberaríamos el nodo si no hay más refs
                ctx->rax = 0;
            } else {
                ctx->rax = -1;
            }
            break;
        }

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
