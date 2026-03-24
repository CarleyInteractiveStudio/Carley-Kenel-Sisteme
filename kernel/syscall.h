#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "sched.h"

#define SYS_YIELD     0
#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2
#define SYS_OPEN      3
#define SYS_READ      4
#define SYS_CLOSE     5
#define SYS_WRITE     6
#define SYS_READDIR   7
#define SYS_EXIT      8
#define SYS_SBRK      9
#define SYS_SPAWN     10
#define SYS_GET_INFO  11

void syscall_init(void);
context_t *syscall_handler(context_t *ctx);

#endif
