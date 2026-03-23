#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include "common/limine.h"

/* Estructura para el servidor de video */
void video_init(struct limine_framebuffer *fb);

/* Funciones de dibujo básico */
void video_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void video_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void video_clear(uint32_t color);

/* Funciones de texto */
void video_draw_char(char c, uint32_t x, uint32_t y, uint32_t color);
void video_draw_string(const char *str, uint32_t x, uint32_t y, uint32_t color);

#endif
