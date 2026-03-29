#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

typedef struct {
    uint64_t framebuffer_address;
    uint32_t screen_width;
    uint32_t screen_height;
    uint64_t memory_map_address;
    uint32_t memory_map_count;
} __attribute__((packed)) boot_info_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attr;
} __attribute__((packed)) e820_entry_t;

#endif
