#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <stddef.h>

/* Estructura de mensaje básico */
typedef struct {
    uint64_t sender;
    uint64_t type;
    uint64_t data[4]; // 32 bytes de datos útiles
} ipc_msg_t;

/* Envía un mensaje a una tarea específica */
int ipc_send(uint64_t dest_id, ipc_msg_t *msg);

/* Recibe un mensaje (bloqueante por ahora) */
int ipc_recv(ipc_msg_t *msg);

#endif
