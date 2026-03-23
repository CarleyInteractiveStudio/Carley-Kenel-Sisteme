#include "sched.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "gdt.h"
#include "common/string.h"
#include "common/limine.h"

#define STACK_SIZE (PAGE_SIZE * 2)
#define DEFAULT_USER_HEAP_START 0x80000000000

extern volatile struct limine_hhdm_request hhdm_request;

static task_t *current_task = NULL;
static task_t *task_list = NULL;
static uint64_t next_id = 1;

void sched_init(void) {
    current_task = kmalloc(sizeof(task_t));
    current_task->id = 0;
    current_task->state = TASK_RUNNING;
    current_task->pml4 = vmm_get_kernel_pagemap();
    current_task->heap_end = 0;
    current_task->next = current_task;
    task_list = current_task;
}

task_t *sched_create_task(void (*entry)(void), bool user) {
    task_t *new_task = kmalloc(sizeof(task_t));
    new_task->id = next_id++;
    new_task->state = TASK_READY;

    if (user) {
        new_task->pml4 = vmm_create_pagemap();
        new_task->heap_end = DEFAULT_USER_HEAP_START;
    } else {
        new_task->pml4 = vmm_get_kernel_pagemap();
        new_task->heap_end = 0;
    }

    uint64_t hhdm = hhdm_request.response->offset;

    /* Pila de Kernel (Siempre necesaria para interrupciones) */
    void *kstack_phys = pmm_alloc_pages(2);
    new_task->kernel_stack = (void *)((uintptr_t)kstack_phys + hhdm);

    uintptr_t stack_virt;
    uintptr_t stack_access_ptr; // Puntero para acceder a la pila desde el kernel

    if (user) {
        /* Pila de Usuario */
        void *ustack_phys = pmm_alloc_pages(2);
        stack_virt = 0x70000000000;
        for(size_t i = 0; i < 2; i++) {
            vmm_map(new_task->pml4, stack_virt + (i * PAGE_SIZE), (uintptr_t)ustack_phys + (i * PAGE_SIZE), PTE_PRESENT | PTE_WRITABLE | PTE_USER);
        }
        /* El kernel accede a la pila de usuario vía HHDM para inicializar el contexto */
        stack_access_ptr = (uintptr_t)ustack_phys + hhdm;
    } else {
        /* Pila de Kernel para tareas de kernel */
        stack_virt = (uintptr_t)new_task->kernel_stack;
        stack_access_ptr = stack_virt;
    }

    new_task->stack_base = (void *)stack_virt;

    /* Inicializar el contexto en el tope de la pila (usando el puntero de acceso HHDM) */
    context_t *ctx = (context_t *)(stack_access_ptr + STACK_SIZE - sizeof(context_t));
    memset(ctx, 0, sizeof(context_t));

    ctx->rip = (uint64_t)entry;
    ctx->rsp = (uint64_t)(stack_virt + STACK_SIZE - sizeof(context_t));
    ctx->rflags = 0x202;

    if (user) {
        ctx->cs = 0x1B;
        ctx->ss = 0x23;
    } else {
        ctx->cs = 0x08;
        ctx->ss = 0x10;
    }

    new_task->context = (context_t *)(stack_virt + STACK_SIZE - sizeof(context_t));
    new_task->next = task_list->next;
    task_list->next = new_task;

    return new_task;
}

context_t *sched_schedule(context_t *current_context) {
    if (!current_task) return current_context;
    current_task->context = current_context;
    if (current_task->state == TASK_RUNNING) current_task->state = TASK_READY;

    task_t *next_task = current_task->next;
    while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
        if (next_task == current_task && current_task->state == TASK_DEAD) {
            for (;;) __asm__("hlt");
        }
    }

    current_task = next_task;
    current_task->state = TASK_RUNNING;

    vmm_switch_pagemap(current_task->pml4);
    tss_set_rsp0((uint64_t)current_task->kernel_stack + STACK_SIZE);

    return current_task->context;
}

void sched_yield(void) {
    __asm__ volatile("int $32");
}

void sched_terminate_task(void) {
    if (current_task) {
        current_task->state = TASK_DEAD;
    }
}

task_t *sched_get_current_task(void) {
    return current_task;
}
