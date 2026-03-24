#include "sched.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "gdt.h"
#include "common/string.h"
#include "common/limine.h"
#include "spinlock.h"

#define STACK_SIZE (PAGE_SIZE * 2)
#define DEFAULT_USER_HEAP_START 0x80000000000

extern volatile struct limine_hhdm_request hhdm_request;

/* Multi-core: Un puntero por CPU para saber qué corre en cada una */
static task_t *current_tasks[256];
static task_t *task_list = NULL;
static uint64_t next_id = 1;
static spinlock_t sched_lock = 0;

/* Ayudante para obtener ID de CPU actual vía Limine */
extern volatile struct limine_smp_request smp_request;
static uint64_t get_cpu_id(void) {
    if (!smp_request.response) return 0;
    /* En x86_64 real usaríamos GS base o local APIC ID */
    /* Para esta demo, simulamos BSP=0 */
    return 0;
}

void sched_init(void) {
    task_t *kernel_idle = kmalloc(sizeof(task_t));
    memset(kernel_idle, 0, sizeof(task_t));
    kernel_idle->id = 0;
    kernel_idle->state = TASK_RUNNING;
    kernel_idle->pml4 = vmm_get_kernel_pagemap();
    kernel_idle->next = kernel_idle;
    task_list = kernel_idle;

    for(int i=0; i<256; i++) current_tasks[i] = kernel_idle;
}

task_t *sched_create_task(void (*entry)(void), bool user) {
    spin_lock(&sched_lock);
    task_t *new_task = kmalloc(sizeof(task_t));
    memset(new_task, 0, sizeof(task_t));
    new_task->id = next_id++;
    new_task->state = TASK_READY;

    if (user) {
        new_task->pml4 = vmm_create_pagemap();
        new_task->heap_end = DEFAULT_USER_HEAP_START;
    } else {
        new_task->pml4 = vmm_get_kernel_pagemap();
    }

    uint64_t hhdm = hhdm_request.response->offset;
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
    uint64_t cpu = get_cpu_id();

    spin_lock(&sched_lock);
    if (!current_tasks[cpu]) { spin_unlock(&sched_lock); return current_context; }

    current_tasks[cpu]->context = current_context;
    if (current_tasks[cpu]->state == TASK_RUNNING) current_tasks[cpu]->state = TASK_READY;

    task_t *next_task = current_tasks[cpu]->next;
    while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
        if (next_task == current_tasks[cpu] && current_tasks[cpu]->state == TASK_DEAD) {
            spin_unlock(&sched_lock);
            for (;;) __asm__("hlt");
        }
    }

    current_tasks[cpu] = next_task;
    current_tasks[cpu]->state = TASK_RUNNING;

    vmm_switch_pagemap(current_tasks[cpu]->pml4);
    tss_set_rsp0((uint64_t)current_tasks[cpu]->kernel_stack + STACK_SIZE);

    spin_unlock(&sched_lock);
    return current_tasks[cpu]->context;
}

void sched_yield(void) { __asm__ volatile("int $32"); }
void sched_terminate_task(void) {
    uint64_t cpu = get_cpu_id();
    if (current_tasks[cpu]) current_tasks[cpu]->state = TASK_DEAD;
}
task_t *sched_get_current_task(void) { return current_tasks[get_cpu_id()]; }

task_t *sched_get_task_by_id(uint64_t id) {
    task_t *curr = task_list;
    do {
        if (curr->id == id) return curr;
        curr = curr->next;
    } while (curr != task_list);
    return NULL;
}
