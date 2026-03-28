#ifndef IPC_H
#define IPC_H

#include <stdint.h>

int ipc_send(uint64_t dest_id, void *msg);
int ipc_recv(void *msg);

#endif
