#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Higher Half Direct Map Offset (64-bit canonical address) */
#define HHDM_OFFSET 0xFFFF800000000000ULL

/* Virtual Base Address for the Kernel */
#define KERNEL_VIRT_BASE (HHDM_OFFSET + 0x100000)

/* Page Size (4KB) */
#define PAGE_SIZE 4096

/* CarleyFS Constants - Synchronized with format_carleyfs and stage2 */
#define CARLEYFS_MAGIC 0xCA121E1
#define CARLEYFS_SUPERBLOCK_SECTOR 64
#define CARLEYFS_INODE_SECTOR 65
#define CARLEYFS_DATA_SECTOR 96
#define CARLEYFS_INITRD_SECTOR 20480
#define CARLEYFS_MAX_INODES 64

#endif
