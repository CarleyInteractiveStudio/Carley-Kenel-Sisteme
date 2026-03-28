#include "io.h"
#include "ipc.h"
#include "keyboard_buf.h"
#include "sched.h" // Para ipc_msg_t
#include "string.h"

static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
  '9', '0', '-', '=', '\b',
  '\t',
  'q', 'w', 'e', 'r',
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
 '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',
  'm', ',', '.', '/',   0,
  '*',
    0,
  ' ',
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    if (scancode < 0x80) {
        char key = scancode_to_ascii[scancode];
        if (key != 0) {
            kbd_buf_push(key);

            ipc_msg_t msg;
            msg.sender = 100;
            msg.type = 0x11;
            msg.data[0] = (uint64_t)key;
            ipc_send(0, &msg);
        }
    }
}
