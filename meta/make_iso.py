#!/usr/bin/env python3
import sys
import os
import subprocess

def build_bootable_media(input_bin, output_img, output_iso):
    if not os.path.exists(input_bin):
        print(f"Error: {input_bin} does not exist.")
        sys.exit(1)

    # 1. Compile 16-bit to 32-bit BIOS bootsector stub
    bootsector_src = "meta/bootsector.s"
    bootsector_bin = "meta/bootsector.bin"

    cmd_as = f"as --32 {bootsector_src} -o meta/bootsector.o"
    cmd_ld = f"ld -m elf_i386 -Ttext 0x7C00 --oformat binary meta/bootsector.o -o {bootsector_bin}"

    subprocess.run(cmd_as, shell=True, check=True)
    subprocess.run(cmd_ld, shell=True, check=True)

    with open(bootsector_bin, 'rb') as f_boot:
        boot_data = f_boot.read()

    with open(input_bin, 'rb') as f_kernel:
        kernel_data = f_kernel.read()

    # 2. Build carley-os.img (Bootable disk image: Sector 0 bootsector + Sector 1+ OS kernel)
    raw_disk = boot_data + kernel_data
    # Pad to standard 1.44MB floppy/HDD size (1474560 bytes)
    img_target_size = 1474560
    if len(raw_disk) < img_target_size:
        raw_disk = raw_disk + b'\x00' * (img_target_size - len(raw_disk))

    with open(output_img, 'wb') as f_img:
        f_img.write(raw_disk)

    print(f"[Boot Media Builder] Successfully created bootable disk image: {output_img} ({len(raw_disk)} bytes)")

    # 3. Build carley-os.iso (ISO9660 El Torito Bootable CD-ROM image)
    SECTOR_SIZE = 2048

    # System Area (32KB = 16 sectors)
    iso_data = bytearray(16 * SECTOR_SIZE)

    # Primary Volume Descriptor (Sector 16)
    pvd = bytearray(SECTOR_SIZE)
    pvd[0] = 1 # Type: PVD
    pvd[1:6] = b'CD001' # Standard ID
    pvd[6] = 1 # Version
    pvd[40:72] = b'CARLEY_OS'.ljust(32, b' ') # Volume Identifier
    iso_data.extend(pvd)

    # Volume Descriptor Set Terminator (Sector 17)
    vdt = bytearray(SECTOR_SIZE)
    vdt[0] = 255 # Type: Terminator
    vdt[1:6] = b'CD001'
    vdt[6] = 1
    iso_data.extend(vdt)

    # Boot Record Sector (Sector 18) - El Torito
    brs = bytearray(SECTOR_SIZE)
    brs[0] = 0 # Boot Record
    brs[1:6] = b'CD001'
    brs[6] = 1
    brs[7:39] = b'EL TORITO SPECIFICATION'.ljust(32, b'\x00')
    iso_data.extend(brs)

    # Boot payload
    payload = raw_disk
    payload_sectors = (len(payload) + SECTOR_SIZE - 1) // SECTOR_SIZE
    payload_padded = payload + b'\x00' * (payload_sectors * SECTOR_SIZE - len(payload))
    iso_data.extend(payload_padded)

    with open(output_iso, 'wb') as f_iso:
        f_iso.write(iso_data)

    print(f"[Boot Media Builder] Successfully created bootable ISO image: {output_iso} ({len(iso_data)} bytes)")

if __name__ == '__main__':
    inp = sys.argv[1] if len(sys.argv) > 1 else 'sistema_operativo/cos.bin'
    img = sys.argv[2] if len(sys.argv) > 2 else 'carley-os.img'
    iso = sys.argv[3] if len(sys.argv) > 3 else 'carley-os.iso'
    build_bootable_media(inp, img, iso)
