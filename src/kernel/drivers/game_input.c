#include "game_input.h"
#include "kernel/keyboard_buf.h"

void game_input_update(controller_t *c) {
    /* Resetear estados momentáneos */
    c->up = c->down = c->left = c->right = 0;
    c->btn_a = c->btn_b = 0;

    char key;
    while ((key = kbd_buf_pop()) != 0) {
        if (key == 'w') c->up = 1;
        if (key == 's') c->down = 1;
        if (key == 'a') c->left = 1;
        if (key == 'd') c->right = 1;
        if (key == ' ') c->btn_a = 1;
        if (key == 'x') c->btn_b = 1;
    }
}
