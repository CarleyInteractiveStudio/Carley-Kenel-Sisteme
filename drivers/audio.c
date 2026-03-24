#include "audio.h"
#include "kernel/io.h"

#define SB16_DSP_RESET  (SB16_BASE + 0x06)
#define SB16_DSP_READ   (SB16_BASE + 0x0A)
#define SB16_DSP_WRITE  (SB16_BASE + 0x0C)

static void dsp_write(uint8_t val) {
    while (inb(SB16_DSP_WRITE) & 0x80);
    outb(SB16_DSP_WRITE, val);
}

void audio_init(void) {
    outb(SB16_DSP_RESET, 1);
    for(int i=0; i<1000; i++) __asm__("pause");
    outb(SB16_DSP_RESET, 0);

    uint32_t timeout = 10000;
    while (timeout--) {
        if (inb(SB16_DSP_READ) == 0xAA) break;
    }
    dsp_write(0xD1); // Turn speaker on
}

void audio_play(uint8_t *buffer, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        dsp_write(0x10);
        dsp_write(buffer[i]);
        /* Retardo para ~8kHz */
        for(int j=0; j<2000; j++) __asm__("pause");
    }
}
