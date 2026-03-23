#include "ipc.h"
#include "sched.h"
#include "common/string.h"
#include "kheap.h"
#include "spinlock.h"

typedef struct ipc_node {
    ipc_msg_t msg;
    struct ipc_node *next;
} ipc_node_t;

static ipc_node_t *global_queue = NULL;
static spinlock_t ipc_lock = 0;

int ipc_send(uint64_t dest_id, ipc_msg_t *msg) {
    (void)dest_id;
    spin_lock(&ipc_lock);

    ipc_node_t *node = kmalloc(sizeof(ipc_node_t));
    if (!node) {
        spin_unlock(&ipc_lock);
        return -1;
    }

    memcpy(&node->msg, msg, sizeof(ipc_msg_t));
    node->next = NULL;

    if (!global_queue) {
        global_queue = node;
    } else {
        ipc_node_t *curr = global_queue;
        while (curr->next) curr = curr->next;
        curr->next = node;
    }

    spin_unlock(&ipc_lock);
    return 0;
}

int ipc_recv(ipc_msg_t *msg) {
    spin_lock(&ipc_lock);
    if (!global_queue) {
        spin_unlock(&ipc_lock);
        return -1;
    }

    ipc_node_t *node = global_queue;
    memcpy(msg, &node->msg, sizeof(ipc_msg_t));
    global_queue = node->next;
    kfree(node);

    spin_unlock(&ipc_lock);
    return 0;
}
