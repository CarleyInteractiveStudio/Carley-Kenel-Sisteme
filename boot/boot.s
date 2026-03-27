.code16
.global _start
_start:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp

    # Cargar Stage 2 (suponemos que esta en el segundo sector del disco)
    # Usando interrupcion BIOS 0x13 para leer 16 sectores
    mov $0x02, %ah       # Leer sectores
    mov $16, %al        # 16 sectores
    mov $0x00, %ch       # Cilindro 0
    mov $0x02, %cl       # Sector 2 (Sector 1 es este MBR)
    mov $0x00, %dh       # Cabeza 0
    # DL ya tiene el numero de disco de arranque pasado por la BIOS
    mov $0x8000, %bx     # Cargar en 0x8000 (ES:BX = 0000:8000)
    int $0x13
    jc error

    jmp 0x8000           # Saltar al Stage 2

error:
    mov $0x0E, %ah
    mov $'E', %al
    int $0x10
    hlt

.org 510
.word 0xAA55
