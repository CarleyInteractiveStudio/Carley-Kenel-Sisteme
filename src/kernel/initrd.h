#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "vfs.h"

void initrd_load_all(void *response); // Obsoleto
void initrd_load_custom(void);

#endif
