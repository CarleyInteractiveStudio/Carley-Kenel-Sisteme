#include "ipc.h"
#include "sched.h"
#include "string.h"
#include "kheap.h"
#include "spinlock.h"

static spinlock_t ipc_lock = 0;

int ipc_send(uint64_t dest_id, void *msg) {
    task_t *dest;
    if (dest_id == 0) dest = sched_get_current_task();
    else if (dest_id < 100) {
        // IDs reservados para servicios del sistema
        // ID 1 = Composer/Graphics Server
        dest = sched_get_task_by_id(dest_id);
    } else {
        dest = sched_get_task_by_id(dest_id);
    }

    if (!dest) return -1;

    ipc_msg_t *m = (ipc_msg_t *)msg;
    spin_lock(&ipc_lock);
    struct ipc_msg_node *node = kmalloc(sizeof(struct ipc_msg_node));
    if (!node) { spin_unlock(&ipc_lock); return -1; }

    node->sender = m->sender;
    node->type = m->type;
    memcpy(node->data, m->data, sizeof(uint64_t) * 5);
    node->next = NULL;

    if (!dest->msg_queue) {
        dest->msg_queue = node;
    } else {
        struct ipc_msg_node *curr = dest->msg_queue;
        while (curr->next) curr = curr->next;
        curr->next = node;
    }
    spin_unlock(&ipc_lock);
    return 0;
}

int ipc_recv(void *msg) {
    task_t *curr_task = sched_get_current_task();
    ipc_msg_t *m = (ipc_msg_t *)msg;
    spin_lock(&ipc_lock);
    if (!curr_task->msg_queue) { spin_unlock(&ipc_lock); return -1; }

    struct ipc_msg_node *node = curr_task->msg_queue;
    m->sender = node->sender;
    m->type = node->type;
    memcpy(m->data, node->data, sizeof(uint64_t) * 5);
    curr_task->msg_queue = node->next;
    kfree(node);
    spin_unlock(&ipc_lock);
    return 0;
}
