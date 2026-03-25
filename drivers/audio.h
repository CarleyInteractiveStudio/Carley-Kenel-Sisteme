#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define SB16_BASE 0x220

void audio_init(void);

/* Reproduce un buffer de audio raw (8-bit mono) */
void audio_play(uint8_t *buffer, uint32_t size);
void audio_mixer_step(void);

#endif
