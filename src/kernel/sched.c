#include "sched.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "gdt.h"
#include "string.h"
#include "spinlock.h"
#include "cpu.h"

#define STACK_SIZE (PAGE_SIZE * 2)
#define DEFAULT_USER_HEAP_START 0x80000000000
#define HHDM_OFFSET 0

static task_t *task_list = NULL;
static uint64_t next_id = 1;
static spinlock_t sched_lock = 0;

uint64_t get_cpu_id(void) {
    return cpu_get_local()->cpu_id;
}

void sched_init(void) {
    task_t *kernel_idle = kmalloc(sizeof(task_t));
    memset(kernel_idle, 0, sizeof(task_t));
    kernel_idle->id = 0; // Kernel Idle
    kernel_idle->state = TASK_RUNNING;
    kernel_idle->cpu_id = 0;
    kernel_idle->pml4 = vmm_get_kernel_pagemap();
    kernel_idle->next = kernel_idle;
    task_list = kernel_idle;

    cpu_get_local()->current_task = kernel_idle;
}

task_t *sched_create_task(void (*entry)(void), bool user) {
    spin_lock(&sched_lock);
    task_t *new_task = kmalloc(sizeof(task_t));
    memset(new_task, 0, sizeof(task_t));
    new_task->id = next_id++;
    new_task->state = TASK_READY;
    new_task->cpu_id = -1;

    if (user) {
        new_task->pml4 = vmm_create_pagemap();
        new_task->heap_end = DEFAULT_USER_HEAP_START;
    } else {
        new_task->pml4 = vmm_get_kernel_pagemap();
    }

    uint64_t hhdm = HHDM_OFFSET;
    void *kstack_phys = pmm_alloc_pages(2);
    new_task->kernel_stack = (void *)((uintptr_t)kstack_phys + hhdm);

    uintptr_t stack_virt;
    uintptr_t stack_access_ptr;
    if (user) {
        void *ustack_phys = pmm_alloc_pages(2);
        stack_virt = 0x70000000000;
        for(size_t i = 0; i < 2; i++) {
            vmm_map(new_task->pml4, stack_virt + (i * PAGE_SIZE), (uintptr_t)ustack_phys + (i * PAGE_SIZE), PTE_PRESENT | PTE_WRITABLE | PTE_USER);
        }
        stack_access_ptr = (uintptr_t)ustack_phys + hhdm;
    } else {
        stack_virt = (uintptr_t)new_task->kernel_stack;
        stack_access_ptr = stack_virt;
    }

    new_task->stack_base = (void *)stack_virt;
    context_t *ctx = (context_t *)((uintptr_t)new_task->kernel_stack + STACK_SIZE - sizeof(context_t));
    memset(ctx, 0, sizeof(context_t));
    ctx->rip = (uint64_t)entry;
    ctx->rsp = (uint64_t)(stack_virt + STACK_SIZE - 16);
    ctx->rflags = 0x202;
    if (user) { ctx->cs = 0x1B; ctx->ss = 0x23; }
    else { ctx->cs = 0x08; ctx->ss = 0x10; }

    new_task->context = ctx;
    new_task->next = task_list->next;
    task_list->next = new_task;
    spin_unlock(&sched_lock);
    return new_task;
}

context_t *sched_schedule(context_t *current_context) {
    cpu_local_t *local = cpu_get_local();
    task_t *curr = (task_t *)local->current_task;
    uint64_t my_cpu = local->cpu_id;

    spin_lock(&sched_lock);
    curr->context = current_context;
    __asm__ volatile("fxsave %0" : : "m"(curr->fpu_state));

    if (curr->state == TASK_RUNNING) {
        curr->state = TASK_READY;
        curr->cpu_id = -1;
    }

    task_t *next_task = curr->next;
    while (true) {
        if (next_task->state == TASK_READY && next_task->cpu_id == -1) {
            break;
        }
        next_task = next_task->next;
        if (next_task == curr) {
            // No hay tareas READY libres. Si la actual sigue viva, la retomamos.
            if (curr->state != TASK_DEAD) {
                next_task = curr;
                break;
            } else {
                // El CPU debe quedar IDLE (en un sistema real usaríamos una tarea idle por CPU)
                spin_unlock(&sched_lock);
                for (;;) __asm__("hlt");
            }
        }
    }

    local->current_task = next_task;
    next_task->state = TASK_RUNNING;
    next_task->cpu_id = my_cpu;
    __asm__ volatile("fxrstor %0" : : "m"(next_task->fpu_state));

    vmm_switch_pagemap(next_task->pml4);
    tss_set_rsp0((uint64_t)next_task->kernel_stack + STACK_SIZE);
    spin_unlock(&sched_lock);
    return next_task->context;
}

void sched_yield(void) { __asm__ volatile("int $32"); }
void sched_terminate_task(void) { sched_get_current_task()->state = TASK_DEAD; }
task_t *sched_get_current_task(void) { return (task_t *)cpu_get_local()->current_task; }

task_t *sched_get_task_by_id(uint64_t id) {
    task_t *curr = task_list;
    do {
        if (curr->id == id) return curr;
        curr = curr->next;
    } while (curr != task_list);
    return NULL;
}
