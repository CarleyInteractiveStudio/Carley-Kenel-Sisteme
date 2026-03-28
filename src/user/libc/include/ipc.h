#ifndef IPC_H
#define IPC_H

#include <stdint.h>

typedef struct {
    uint64_t sender;
    uint64_t type;
    uint64_t data[5];
} ipc_msg_t;

#endif
