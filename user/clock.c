#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ipc.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);
extern long syscall1(int num, long arg1);

#define SYS_IPC_SEND 1
#define SYS_TIME 12

#define COMPOSER_DRAW_RECT 2
#define COMPOSER_DRAW_CHAR 3

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
} rtc_time_t;

void main(void) {
    rtc_time_t t;

    printf("Carley Clock v0.1 iniciado.\n");

    for (;;) {
        syscall1(SYS_TIME, (long)&t);

        /* Crear cadena de tiempo rudimentaria (printf no tiene formateo complejo) */
        // Usamos una simulación de dibujo en ventana
        ipc_msg_t msg;
        msg.sender = 300;
        msg.type = COMPOSER_DRAW_RECT;
        msg.data[0] = 400; msg.data[1] = 50; msg.data[2] = 150; msg.data[3] = 40; msg.data[4] = 0x2C3E50;
        syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);

        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = 410; msg.data[2] = 65; msg.data[3] = 0xECF0F1;

        /* Dibujar HH:MM:SS */
        char clock_buf[10];
        clock_buf[0] = (t.hour / 10) + '0';
        clock_buf[1] = (t.hour % 10) + '0';
        clock_buf[2] = ':';
        clock_buf[3] = (t.minute / 10) + '0';
        clock_buf[4] = (t.minute % 10) + '0';
        clock_buf[5] = ':';
        clock_buf[6] = (t.second / 10) + '0';
        clock_buf[7] = (t.second % 10) + '0';
        clock_buf[8] = 0;

        for (int i=0; clock_buf[i]; i++) {
            msg.data[0] = clock_buf[i];
            syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);
            msg.data[1] += 8;
        }

        /* Esperar 1 segundo aproximadamente */
        for(int j=0; j<20000000; j++) __asm__("pause");
        __asm__ volatile("int $0x80" : : "a"(0)); // Yield
    }
}
