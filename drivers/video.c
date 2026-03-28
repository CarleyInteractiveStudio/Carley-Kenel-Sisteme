#include "video.h"
#include "common/string.h"
#include <stdbool.h>

static struct limine_framebuffer *framebuffer;
static uint32_t cursor_x = 10;
static uint32_t cursor_y = 10;
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 10
#define MARGIN 10

static const uint8_t font8x8_basic[128][8] = {
    ['A'] = {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},
    ['B'] = {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},
    ['C'] = {0x1E, 0x33, 0x30, 0x30, 0x30, 0x33, 0x1E, 0x00},
    ['D'] = {0x3E, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00},
    ['E'] = {0x7F, 0x30, 0x30, 0x3E, 0x30, 0x30, 0x7F, 0x00},
    ['F'] = {0x7F, 0x30, 0x30, 0x3E, 0x30, 0x30, 0x30, 0x00},
    ['G'] = {0x1E, 0x33, 0x30, 0x30, 0x37, 0x33, 0x1E, 0x00},
    ['H'] = {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},
    ['I'] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},
    ['J'] = {0x1E, 0x06, 0x06, 0x06, 0x06, 0x36, 0x1C, 0x00},
    ['K'] = {0x33, 0x36, 0x3C, 0x38, 0x3C, 0x36, 0x33, 0x00},
    ['L'] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3F, 0x00},
    ['M'] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00},
    ['N'] = {0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, 0x63, 0x00},
    ['O'] = {0x1E, 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x00},
    ['P'] = {0x3E, 0x66, 0x66, 0x3E, 0x30, 0x30, 0x30, 0x00},
    ['Q'] = {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x0E, 0x00},
    ['R'] = {0x3E, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x66, 0x00},
    ['S'] = {0x1E, 0x33, 0x07, 0x0E, 0x1C, 0x33, 0x1E, 0x00},
    ['T'] = {0x3F, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00},
    ['U'] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x00},
    ['V'] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},
    ['W'] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},
    ['X'] = {0x63, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x63, 0x00},
    ['Y'] = {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x0C, 0x00},
    ['Z'] = {0x3F, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x3F, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['a'] = {0x00, 0x00, 0x1E, 0x03, 0x1F, 0x33, 0x1F, 0x00},
    ['0'] = {0x1E, 0x33, 0x3B, 0x3F, 0x37, 0x33, 0x1E, 0x00},
    ['1'] = {0x0C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06},
    ['/'] = {0x00, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00},
    ['>'] = {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},
};

void video_init(struct limine_framebuffer *fb) { framebuffer = fb; }

void video_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= framebuffer->width || y >= framebuffer->height) return;
    ((uint32_t *)framebuffer->address)[y * (framebuffer->pitch / 4) + x] = color;
}

void video_put_pixel_alpha(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= framebuffer->width || y >= framebuffer->height) return;

    uint8_t alpha = (color >> 24) & 0xFF;
    if (alpha == 255) {
        video_put_pixel(x, y, color);
        return;
    }
    if (alpha == 0) return;

    uint32_t bg = ((uint32_t *)framebuffer->address)[y * (framebuffer->pitch / 4) + x];

    uint32_t rb = ((color & 0xFF00FF) * alpha + (bg & 0xFF00FF) * (255 - alpha)) / 255;
    uint32_t g  = ((color & 0x00FF00) * alpha + (bg & 0x00FF00) * (255 - alpha)) / 255;

    ((uint32_t *)framebuffer->address)[y * (framebuffer->pitch / 4) + x] = (rb & 0xFF00FF) | (g & 0x00FF00);
}

void video_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    bool has_alpha = ((color >> 24) & 0xFF) < 255;
    for (uint32_t i = y; i < y + h; i++) {
        for (uint32_t j = x; j < x + w; j++) {
            if (has_alpha) video_put_pixel_alpha(j, i, color);
            else video_put_pixel(j, i, color);
        }
    }
}

void video_clear(uint32_t color) {
    video_draw_rect(0, 0, framebuffer->width, framebuffer->height, color);
    cursor_x = MARGIN; cursor_y = MARGIN;
}

void video_scroll(void) {
    uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
    uint32_t pitch_w = framebuffer->pitch / 4;
    for (uint32_t y = MARGIN; y < framebuffer->height - CHAR_HEIGHT - MARGIN; y++)
        memcpy(&fb_ptr[y * pitch_w], &fb_ptr[(y + CHAR_HEIGHT) * pitch_w], framebuffer->width * 4);
    video_draw_rect(0, framebuffer->height - CHAR_HEIGHT - MARGIN, framebuffer->width, CHAR_HEIGHT, 0x1E1E1E);
    cursor_y -= CHAR_HEIGHT;
}

void video_draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    video_draw_char_ex(c, x, y, color, 1);
}

void video_draw_char_ex(char c, uint32_t x, uint32_t y, uint32_t color, uint32_t scale) {
    if (c < 0 || (uint32_t)c > 127) return;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (font8x8_basic[(int)c][i] & (1 << (7 - j))) {
                if (scale == 1) video_put_pixel(x + j, y + i, color);
                else video_draw_rect(x + j * scale, y + i * scale, scale, scale, color);
            }
        }
    }
}

void video_terminal_write(char c, uint32_t color) {
    if (c == '\n') { cursor_x = MARGIN; cursor_y += CHAR_HEIGHT; }
    else if (c == '\b') { if (cursor_x > MARGIN) { cursor_x -= CHAR_WIDTH; video_draw_rect(cursor_x, cursor_y, CHAR_WIDTH, CHAR_HEIGHT, 0x1E1E1E); } }
    else { video_draw_char(c, cursor_x, cursor_y, color); cursor_x += CHAR_WIDTH; }
    if (cursor_x > framebuffer->width - MARGIN) { cursor_x = MARGIN; cursor_y += CHAR_HEIGHT; }
    if (cursor_y > framebuffer->height - CHAR_HEIGHT - MARGIN) video_scroll();
}

void video_draw_string(const char *str, uint32_t x, uint32_t y, uint32_t color) {
    while (*str) { video_draw_char(*str++, x, y, color); x += CHAR_WIDTH; }
}

void video_draw_rounded_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r, uint32_t color) {
    for (uint32_t i = y; i < y + h; i++) {
        for (uint32_t j = x; j < x + w; j++) {
            uint32_t dx = (j < x + r) ? (x + r - j) : ((j >= x + w - r) ? (j - (x + w - r) + 1) : 0);
            uint32_t dy = (i < y + r) ? (y + r - i) : ((i >= y + h - r) ? (i - (y + h - r) + 1) : 0);

            if (dx && dy && (dx * dx + dy * dy > r * r)) continue;
            video_put_pixel_alpha(j, i, color);
        }
    }
}

void video_draw_shadow(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t r) {
    // Sombra muy simple, un rectangulo negro con baja opacidad
    video_draw_rounded_rect(x + 5, y + 5, w, h, r, 0x44000000);
}
