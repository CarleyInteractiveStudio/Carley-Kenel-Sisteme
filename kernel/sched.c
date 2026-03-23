#include "sched.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "common/string.h" // Corregido el include path
#include "common/limine.h"

#define STACK_SIZE (PAGE_SIZE * 2)

/* Solicitud para el Higher Half Direct Map (HHDM) de Limine */
extern volatile struct limine_hhdm_request hhdm_request;

static task_t *current_task = NULL;
static task_t *task_list = NULL;
static uint64_t next_id = 1;

void sched_init(void) {
    /* La tarea actual es el hilo de ejecución principal del kernel (kmain) */
    current_task = kmalloc(sizeof(task_t));
    current_task->id = 0;
    current_task->state = TASK_RUNNING;
    current_task->pml4 = vmm_get_kernel_pagemap();
    current_task->next = current_task; // Lista circular simple
    task_list = current_task;
}

task_t *sched_create_task(void (*entry)(void)) {
    task_t *new_task = kmalloc(sizeof(task_t));
    new_task->id = next_id++;
    new_task->state = TASK_READY;
    new_task->pml4 = vmm_get_kernel_pagemap();

    /* Reservar pila para la tarea */
    void *stack_phys = pmm_alloc_pages(2);

    /* Obtener el offset HHDM dinámico de Limine */
    uint64_t hhdm_offset = hhdm_request.response->offset;
    uintptr_t stack_virt = (uintptr_t)stack_phys + hhdm_offset;
    new_task->stack_base = (void *)stack_virt;

    /* Preparar el contexto inicial en el tope de la pila */
    context_t *ctx = (context_t *)(stack_virt + STACK_SIZE - sizeof(context_t));
    memset(ctx, 0, sizeof(context_t));

    ctx->rip = (uint64_t)entry;
    ctx->cs = 0x08;      // Código Kernel
    ctx->ss = 0x10;      // Datos Kernel
    ctx->rsp = (uint64_t)ctx;
    ctx->rflags = 0x202; // Interrupciones habilitadas (IF=1)

    new_task->context = ctx;

    /* Agregar a la lista circular */
    new_task->next = task_list->next;
    task_list->next = new_task;

    return new_task;
}

context_t *sched_schedule(context_t *current_context) {
    if (!current_task) return current_context;

    /* Guardar el contexto de la tarea actual */
    current_task->context = current_context;
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }

    /* Buscar la siguiente tarea lista (Round Robin) */
    task_t *next_task = current_task->next;
    while (next_task->state != TASK_READY && next_task->state != TASK_RUNNING) {
        next_task = next_task->next;
    }

    current_task = next_task;
    current_task->state = TASK_RUNNING;

    return current_task->context;
}

void sched_yield(void) {
    __asm__ volatile("int $32"); // Forzar una interrupción de timer para llamar al scheduler
}
