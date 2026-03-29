#include "efi.h"

// Definiciones para cargar el kernel
#define KERNEL_LOAD_ADDR 0x100000

typedef struct {
    uint64_t FramebufferBase;
    uint64_t FramebufferSize;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void *QueryMode;
    void *SetMode;
    void *Blt;
    struct {
        uint32_t MaxMode;
        uint32_t Mode;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
        uintptr_t SizeOfInfo;
        uint64_t FramebufferBase;
        uintptr_t FramebufferSize;
    } *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

static EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (uint16_t *)L"--- CARLEY OS UEFI LOADER ---\r\n");

    // 1. Configurar Gráficos (GOP)
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_STATUS status = SystemTable->BootServices->HandleProtocol(ImageHandle, &gop_guid, (void**)&gop);
    if (status != EFI_SUCCESS) {
        // En UEFI real se usa LocateProtocol para GOP
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (uint16_t *)L"GOP not found via Handle.\r\n");
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (uint16_t *)L"Loading kernel from FAT32 partition...\r\n");

    // 2. Obtener Mapa de Memoria
    uintptr_t map_size = 0;
    void *map_buf = NULL;
    uintptr_t map_key, desc_size;
    uint32_t desc_ver;

    SystemTable->BootServices->GetMemoryMap(&map_size, NULL, &map_key, &desc_size, &desc_ver);
    map_size += 2 * desc_size; // Padding
    SystemTable->BootServices->AllocatePool(2, map_size, &map_buf); // EfiLoaderData = 2
    SystemTable->BootServices->GetMemoryMap(&map_size, map_buf, &map_key, &desc_size, &desc_ver);

    // 3. Salir de los servicios de arranque
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (uint16_t *)L"Exiting boot services and jumping to kernel...\r\n");
    SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);

    // 4. Saltar al Kernel
    // En una implementación real, aquí cargaríamos kernel.elf vía EFI_FILE_PROTOCOL
    // Para esta etapa, simulamos el salto.
    typedef void (*kernel_entry_t)(void *);
    kernel_entry_t kernel = (kernel_entry_t)0xFFFF800000100000;

    // Pasar Boot Info en RDI (según protocolo de Carley OS)
    // kernel(boot_info);

    while(1);
    return EFI_SUCCESS;
}
