#include "ipc.h"
#include "sched.h"
#include "common/string.h" // Corregido el include path
#include "kheap.h"

/* Estructura para la cola de mensajes */
typedef struct ipc_node {
    ipc_msg_t msg;
    struct ipc_node *next;
} ipc_node_t;

static ipc_node_t *global_queue = NULL;

int ipc_send(uint64_t dest_id, ipc_msg_t *msg) {
    (void)dest_id; // Por ahora, una cola global compartida para la demo

    ipc_node_t *node = kmalloc(sizeof(ipc_node_t));
    if (!node) return -1;

    memcpy(&node->msg, msg, sizeof(ipc_msg_t));
    node->next = NULL;

    if (!global_queue) {
        global_queue = node;
    } else {
        ipc_node_t *curr = global_queue;
        while (curr->next) curr = curr->next;
        curr->next = node;
    }

    return 0;
}

int ipc_recv(ipc_msg_t *msg) {
    if (!global_queue) return -1; // Cola vacía

    ipc_node_t *node = global_queue;
    memcpy(msg, &node->msg, sizeof(ipc_msg_t));
    global_queue = node->next;
    kfree(node);

    return 0;
}
