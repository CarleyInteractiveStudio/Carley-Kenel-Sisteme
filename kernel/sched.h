#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

/* Estado de un proceso/hilo (Contexto de Interrupción)
   Debe coincidir EXACTAMENTE con el stack frame de isr.s */
typedef struct {
    /* Registros guardados manualmente en isr.s */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;

    /* Empujados por la macro ISR o por la CPU */
    uint64_t int_no, error_code;

    /* Empujados automáticamente por la CPU en iretq */
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) context_t;

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_SLEEPING,
    TASK_DEAD
} task_state_t;

typedef struct task {
    uint64_t id;
    context_t *context;
    void *stack_base;
    uint64_t *pml4;
    task_state_t state;
    struct task *next;
} task_t;

/* Inicializa el planificador */
void sched_init(void);

/* Crea una nueva tarea (hilo) */
task_t *sched_create_task(void (*entry)(void));

/* Selecciona la siguiente tarea a ejecutar (Round Robin) */
context_t *sched_schedule(context_t *current_context);

/* Cede el control voluntariamente (Yield) */
void sched_yield(void);

#endif
