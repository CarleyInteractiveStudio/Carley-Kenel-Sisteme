#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "vfs.h"

struct limine_module_response;

void initrd_load_all(void *response); // Obsoleto
void initrd_load_custom(void);
void initrd_load_limine(struct limine_module_response *response);

#endif
