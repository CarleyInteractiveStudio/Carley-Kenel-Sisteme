#include "syscall.h"
#include "ipc.h"
#include "sched.h"
#include "vfs.h"
#include "drivers/video.h"
#include "drivers/rtc.h"
#include "drivers/audio.h"
#include "keyboard_buf.h"
#include "pmm.h"
#include "vmm.h"
#include "elf.h"
#include "cpu.h"

context_t *syscall_handler(context_t *ctx) {
    uint64_t sys_no = ctx->rax;

    switch (sys_no) {
        case SYS_YIELD: return sched_schedule(ctx);
        case SYS_IPC_SEND: ctx->rax = ipc_send(ctx->rdi, (void *)ctx->rsi); break;
        case SYS_IPC_RECV: ctx->rax = ipc_recv((void *)ctx->rdi); break;
        case SYS_OPEN: ctx->rax = (uintptr_t)vfs_open((const char *)ctx->rdi); break;
        case SYS_READ:
            if (ctx->rdi == 0) ctx->rax = kbd_buf_read((char *)ctx->r10, (size_t)ctx->rdx);
            else ctx->rax = vfs_read((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (uint32_t)ctx->rdx, (uint8_t *)ctx->r10);
            break;
        case SYS_WRITE:
            if (ctx->rdi == 1) {
                char *buf = (char *)ctx->r10;
                for (size_t i = 0; i < ctx->rdx; i++) video_terminal_write(buf[i], 0xFFFFFF);
                ctx->rax = ctx->rdx;
            } else {
                ctx->rax = vfs_write((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (uint32_t)ctx->rdx, (uint8_t *)ctx->r10);
            }
            break;
        case SYS_READDIR: ctx->rax = vfs_readdir((vfs_node_t *)ctx->rdi, (uint32_t)ctx->rsi, (vfs_dirent_t *)ctx->rdx); break;
        case SYS_CLOSE:
            if (ctx->rdi < MAX_FILES_PER_TASK) { sched_get_current_task()->files[ctx->rdi] = NULL; ctx->rax = 0; }
            else { ctx->rax = -1; }
            break;
        case SYS_EXIT: sched_terminate_task(); return sched_schedule(ctx);
        case SYS_SBRK: {
            task_t *curr = sched_get_current_task();
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
            ctx->rax = old_heap; break;
        }
        case SYS_SPAWN: ctx->rax = elf_load_ext((const char *)ctx->rdi, (int)ctx->rsi, (char **)ctx->rdx); break;
        case SYS_GET_INFO:
            if (ctx->rdi == 0) ctx->rax = pmm_get_total_memory();
            else if (ctx->rdi == 1) ctx->rax = pmm_get_free_memory();
            else ctx->rax = 0;
            break;
        case SYS_TIME: rtc_get_time((rtc_time_t *)ctx->rdi); ctx->rax = 0; break;
        case SYS_AUDIO_PLAY: audio_play((uint8_t *)ctx->rdi, (uint32_t)ctx->rsi); ctx->rax = 0; break;

        case SYS_MMAP: {
            uintptr_t addr = (uintptr_t)ctx->rdi;
            size_t len = (size_t)ctx->rsi;
            size_t pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
            for (size_t i = 0; i < pages; i++) {
                void *phys = pmm_alloc_page();
                vmm_map(sched_get_current_task()->pml4, addr + (i * PAGE_SIZE), (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
            }
            ctx->rax = addr; break;
        }

        case SYS_IOPL: {
            /* RDI = level (0-3). Cambia el flag IOPL en RFLAGS para permitir in/out en Ring 3 */
            /* Esto es vital para drivers en espacio de usuario */
            uint64_t level = ctx->rdi & 3;
            ctx->rflags &= ~(3ULL << 12); // Limpiar bits 12-13
            ctx->rflags |= (level << 12);
            ctx->rax = 0;
            break;
        }

        case SYS_CREATE: ctx->rax = vfs_create((const char *)ctx->rdi, (uint32_t)ctx->rsi); break;
        case SYS_MKDIR: ctx->rax = vfs_mkdir((const char *)ctx->rdi); break;

        default: ctx->rax = -1; break;
    }
    return ctx;
}

void syscall_init(void) {}
