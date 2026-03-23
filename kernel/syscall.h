#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "sched.h"

/* Números de Syscalls */
#define SYS_YIELD     0
#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2

/* Inicializa el mecanismo de syscalls (SYSCALL/SYSRET en x86_64) */
void syscall_init(void);

/* Manejador de llamadas al sistema en C */
context_t *syscall_handler(context_t *ctx);

#endif
