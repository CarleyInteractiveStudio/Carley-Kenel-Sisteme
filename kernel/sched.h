#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) context_t;

typedef enum {
    TASK_RUNNING, TASK_READY, TASK_SLEEPING, TASK_DEAD
} task_state_t;

/* Estructura para la cola de mensajes IPC */
struct ipc_msg_node {
    uint64_t sender;
    uint64_t type;
    uint64_t data[4];
    struct ipc_msg_node *next;
};

typedef struct task {
    uint64_t id;
    context_t *context;
    void *stack_base;
    void *kernel_stack;
    uint64_t *pml4;
    uintptr_t heap_end;
    task_state_t state;

    /* Cola de mensajes propia del proceso */
    struct ipc_msg_node *msg_queue;

    struct task *next;
} task_t;

void sched_init(void);
task_t *sched_create_task(void (*entry)(void), bool user);
context_t *sched_schedule(context_t *current_context);
void sched_yield(void);
void sched_terminate_task(void);
task_t *sched_get_current_task(void);
task_t *sched_get_task_by_id(uint64_t id);

#endif
