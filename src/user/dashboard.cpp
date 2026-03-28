#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <bmp.h>
}

/* Dashboard de Carley OS - Estilo "Burbuja de Cristal" (C++) */

#define COMPOSER_DRAW_RECT  2
#define COMPOSER_DRAW_CHAR  3
#define COMPOSER_DRAW_SPRITE 5

extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);
extern "C" long shm_get(uint64_t id, size_t size);
extern "C" void *shm_at(uint64_t shm_id, uintptr_t hint);
#define SYS_IPC_RECV 2

class GlassWidget {
protected:
    uint32_t x, y, w, h;
    uint32_t color;

public:
    GlassWidget(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h, uint32_t _color)
        : x(_x), y(_y), w(_w), h(_h), color(_color) {}

    virtual void draw() {
        ipc_msg_t msg;
        msg.sender = 1001;
        msg.type = COMPOSER_DRAW_RECT;
        msg.data[0] = x; msg.data[1] = y; msg.data[2] = w; msg.data[3] = h; msg.data[4] = color;
        // Enviar al Graphics Server (ID 1)
        syscall3(1, 1, (long)&msg, 0);
    }
};

class AppButton : public GlassWidget {
    char name[16];
    uint32_t *icon_data;
    uint32_t iw, ih;

public:
    AppButton(uint32_t _x, uint32_t _y, const char* _name, const char* icon_path = NULL)
        : GlassWidget(_x, _y, 60, 60, 0x88FFFFFF), x_pos(_x), y_pos(_y), icon_data(NULL) {

        if (icon_path) {
            icon_data = bmp_load(icon_path, &iw, &ih);
            if (icon_data) {
                // Compartir memoria para el icono (Seguridad Microkernel)
                long shm_id = shm_get(1000 + _x, iw * ih * 4);
                uint32_t *shm_ptr = (uint32_t *)shm_at(shm_id, NULL);
                for(uint32_t i=0; i<iw*ih; i++) shm_ptr[i] = icon_data[i];

                ipc_msg_t m;
                m.sender = 1001; m.type = 6; // LOAD_ICON
                m.data[0] = iw; m.data[1] = ih; m.data[2] = shm_id;
                syscall3(1, 1, (long)&m, 0);
            }
        }

        strncmp(name, _name, 15);
        int i=0;
        for(; _name[i] && i < 15; i++) name[i] = _name[i];
        name[i] = 0;
    }

    void draw() override {
        // Fondo translúcido (Efecto cristal)
        GlassWidget::draw();

        if (icon_data) {
            ipc_msg_t m;
            m.sender = 1001; m.type = 5; // DRAW_SPRITE
            m.data[0] = x_pos + 10; m.data[1] = y_pos + 10;
            m.data[2] = iw; m.data[3] = ih; m.data[4] = (uint64_t)icon_data;
            syscall3(1, 1, (long)&m, 0);
        }

        // Texto
        ipc_msg_t msg;
        msg.sender = 1001;
        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = x_pos + 5; msg.data[2] = y_pos + 65; msg.data[3] = 0xFFFFFF;
        for(int i=0; name[i]; i++) {
            msg.data[0] = name[i];
            // Enviar al Graphics Server (ID 1)
            syscall3(1, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
    }
private:
    uint32_t x_pos, y_pos;
};

int main() {
    printf("Dashboard de Carley OS: Iniciando...\n");

    // "Burbuja de cristal" central (Dock)
    GlassWidget dock(250, 500, 300, 80, 0x66444444);

    AppButton btn1(270, 510, "Game", "/disk/game.bmp");
    AppButton btn2(340, 510, "Code", "/disk/code.bmp");
    AppButton btn3(410, 510, "Setup", "/disk/setup.bmp");

    // Registrar ventana para recibir eventos
    ipc_msg_t m;
    m.sender = 1001; m.type = 10; // CREATE_WINDOW
    m.data[0] = 250; m.data[1] = 500; m.data[2] = 300; m.data[3] = 80;
    syscall3(1, 1, (long)&m, 0);

    while (1) {
        dock.draw();
        btn1.draw();
        btn2.draw();
        btn3.draw();

        ipc_msg_t event;
        if (syscall3(SYS_IPC_RECV, 0, (long)&event, 0) == 0) {
            if (event.type == 21) { // WM_CLICK
                uint32_t click_x = event.data[0];
                if (click_x > 20 && click_x < 80) syscall1(10, (long)"game.elf");
                else if (click_x > 90 && click_x < 150) syscall1(10, (long)"code.elf");
                else if (click_x > 160 && click_x < 220) syscall1(10, (long)"setup.elf");
            }
        }

        // Ceder CPU
        __asm__ volatile("int $0x80" : : "a"(0));
    }

    return 0;
}
