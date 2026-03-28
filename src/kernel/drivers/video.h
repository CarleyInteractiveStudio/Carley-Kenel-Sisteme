#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include "limine.h"

void video_init(struct limine_framebuffer *fb);
void video_init_vbe(uint64_t addr, uint32_t w, uint32_t h);
void video_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void video_put_pixel_alpha(uint32_t x, uint32_t y, uint32_t color);
void video_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void video_clear(uint32_t color);
void video_draw_char(char c, uint32_t x, uint32_t y, uint32_t color);
void video_draw_char_ex(char c, uint32_t x, uint32_t y, uint32_t color, uint32_t scale);
void video_draw_string(const char *str, uint32_t x, uint32_t y, uint32_t color);
void video_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color);
void video_draw_shadow(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r);

/* Nuevas funciones para comportamiento de terminal */
void video_scroll(void);
void video_terminal_write(char c, uint32_t color);

#endif
