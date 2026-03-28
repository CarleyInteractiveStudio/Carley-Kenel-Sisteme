#include "audio.h"
#include "kernel/io.h"

#define SB16_DSP_RESET  (SB16_BASE + 0x06)
#define SB16_DSP_READ   (SB16_BASE + 0x0A)
#define SB16_DSP_WRITE  (SB16_BASE + 0x0C)

#define AUDIO_CHANNELS 4
static uint8_t *channels[AUDIO_CHANNELS];
static uint32_t channel_sizes[AUDIO_CHANNELS];
static uint32_t channel_pos[AUDIO_CHANNELS];

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
    /* Registro de canal para mezclador simple */
    for (int i = 0; i < AUDIO_CHANNELS; i++) {
        if (channel_sizes[i] == 0) {
            channels[i] = buffer;
            channel_sizes[i] = size;
            channel_pos[i] = 0;
            return;
        }
    }
}

/* El kernel debería llamar a esta función en un timer o hilo dedicado */
void audio_mixer_step(void) {
    int active = 0;
    uint32_t mix = 0;

    for (int i = 0; i < AUDIO_CHANNELS; i++) {
        if (channel_pos[i] < channel_sizes[i]) {
            mix += channels[i][channel_pos[i]++];
            active++;
        } else {
            channel_sizes[i] = 0;
        }
    }

    if (active > 0) {
        dsp_write(0x10);
        dsp_write((uint8_t)(mix / active));
    }
}
