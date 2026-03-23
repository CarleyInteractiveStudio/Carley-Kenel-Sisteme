#include "sched.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "gdt.h" // Necesario para actualizar el TSS
#include "common/string.h"
#include "common/limine.h"

#define STACK_SIZE (PAGE_SIZE * 2)

extern volatile struct limine_hhdm_request hhdm_request;

static task_t *current_task = NULL;
static task_t *task_list = NULL;
static uint64_t next_id = 1;

void sched_init(void) {
    current_task = kmalloc(sizeof(task_t));
    current_task->id = 0;
    current_task->state = TASK_RUNNING;
    current_task->pml4 = vmm_get_kernel_pagemap();
    current_task->next = current_task;
    task_list = current_task;
}

task_t *sched_create_task(void (*entry)(void), bool user) {
    task_t *new_task = kmalloc(sizeof(task_t));
    new_task->id = next_id++;
    new_task->state = TASK_READY;
    new_task->pml4 = vmm_get_kernel_pagemap();

    void *stack_phys = pmm_alloc_pages(2);
    uint64_t hhdm_offset = hhdm_request.response->offset;
    uintptr_t stack_virt = (uintptr_t)stack_phys + hhdm_offset;
    new_task->stack_base = (void *)stack_virt;

    context_t *ctx = (context_t *)(stack_virt + STACK_SIZE - sizeof(context_t));
    memset(ctx, 0, sizeof(context_t));

    ctx->rip = (uint64_t)entry;
    ctx->rsp = (uint64_t)ctx;
    ctx->rflags = 0x202; // IF=1

    if (user) {
        ctx->cs = 0x1B; // User Code
        ctx->ss = 0x23; // User Data
    } else {
        ctx->cs = 0x08; // Kernel Code
        ctx->ss = 0x10; // Kernel Data
    }

    new_task->context = ctx;
    new_task->next = task_list->next;
    task_list->next = new_task;

    return new_task;
}

context_t *sched_schedule(context_t *current_context) {
    if (!current_task) return current_context;

    current_task->context = current_context;
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }

    task_t *next_task = current_task->next;
    while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
    }

    current_task = next_task;
    current_task->state = TASK_RUNNING;

    /* ACTUALIZACIÓN CRÍTICA: Actualizar la pila del kernel en el TSS */
    /* Cuando ocurra una interrupción en Ring 3, la CPU saltará a esta dirección */
    tss_set_rsp0((uint64_t)current_task->stack_base + STACK_SIZE);

    return current_task->context;
}

void sched_yield(void) {
    __asm__ volatile("int $32");
}
