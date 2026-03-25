#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
}

/* Dashboard de Carley OS - Estilo "Burbuja de Cristal" (C++) */

#define COMPOSER_DRAW_RECT  2
#define COMPOSER_DRAW_CHAR  3
#define COMPOSER_DRAW_SPRITE 5

extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

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
        syscall3(1, 0, (long)&msg, 0);
    }
};

class AppButton : public GlassWidget {
    char name[16];
public:
    AppButton(uint32_t _x, uint32_t _y, const char* _name)
        : GlassWidget(_x, _y, 60, 60, 0x88FFFFFF), x_pos(_x), y_pos(_y) {
        strncmp(name, _name, 15); // Fallback if strncpy missing, but we'll use a loop
        int i=0;
        for(; _name[i] && i < 15; i++) name[i] = _name[i];
        name[i] = 0;
    }

    void draw() override {
        // Fondo translúcido (Efecto cristal)
        GlassWidget::draw();

        // Texto
        ipc_msg_t msg;
        msg.sender = 1001;
        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = x_pos + 5; msg.data[2] = y_pos + 65; msg.data[3] = 0xFFFFFF;
        for(int i=0; name[i]; i++) {
            msg.data[0] = name[i];
            syscall3(1, 0, (long)&msg, 0);
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

    AppButton btn1(270, 510, "Game");
    AppButton btn2(340, 510, "Code");
    AppButton btn3(410, 510, "Settings");

    while (1) {
        dock.draw();
        btn1.draw();
        btn2.draw();
        btn3.draw();

        // Ceder CPU
        __asm__ volatile("int $0x80" : : "a"(0));
        // Pequeño delay
        for(int i=0; i<1000000; i++) __asm__("pause");
    }

    return 0;
}
