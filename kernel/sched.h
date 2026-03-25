#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include "vfs.h"

typedef struct {
    uint64_t sender;
    uint64_t type;
    uint64_t data[5];
} ipc_msg_t;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) context_t;

typedef enum {
    TASK_RUNNING, TASK_READY, TASK_SLEEPING, TASK_DEAD
} task_state_t;

struct ipc_msg_node {
    uint64_t sender;
    uint64_t type;
    uint64_t data[5];
    struct ipc_msg_node *next;
};

#define MAX_FILES_PER_TASK 32

typedef enum {
    CAP_NONE = 0,
    CAP_DISK = (1 << 0),
    CAP_NETWORK = (1 << 1),
    CAP_HARDWARE = (1 << 2),
    CAP_SYS_ADMIN = (1 << 3)
} capability_t;

typedef struct task {
    uint64_t id;
    context_t *context;
    void *stack_base;
    void *kernel_stack;
    uint64_t *pml4;
    uintptr_t heap_end;
    task_state_t state;
    int cpu_id; // -1 if not running, otherwise CPU ID
    struct ipc_msg_node *msg_queue;

    /* Tabla de descriptores de archivos del proceso */
    vfs_node_t *files[MAX_FILES_PER_TASK];
    uint32_t capabilities;

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
