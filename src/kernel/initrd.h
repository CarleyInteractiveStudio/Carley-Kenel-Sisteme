#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "limine.h"
#include "vfs.h"

void initrd_load_all(struct limine_module_response *response);
void initrd_load_custom(void);

#endif
