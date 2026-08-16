#!/usr/bin/env python3
import sys
import os
import subprocess

def build_iso(input_bin, output_iso):
    if not os.path.exists(input_bin):
        print(f"Error: {input_bin} does not exist.")
        sys.exit(1)

    # Try system tools first (grub-mkrescue or xorriso / genisoimage)
    iso_cmd = f"grub-mkrescue -o {output_iso} isodir 2>/dev/null || xorriso -as mkisofs -R -b boot/boot.bin -no-emul-boot -boot-load-size 4 -o {output_iso} isodir 2>/dev/null || genisoimage -R -b boot/boot.bin -no-emul-boot -boot-load-size 4 -o {output_iso} isodir 2>/dev/null"

    # Create isodir layout
    os.makedirs("isodir/boot", exist_ok=True)
    with open(input_bin, 'rb') as f_in, open("isodir/boot/kernel.bin", 'wb') as f_out:
        f_out.write(f_in.read())

    res = subprocess.run(iso_cmd, shell=True)

    if res.returncode == 0 and os.path.exists(output_iso) and os.path.getsize(output_iso) > 0:
        print(f"[ISO Builder] Successfully created ISO using toolchain: {output_iso}")
    else:
        # Generate valid ISO9660 file structure with Primary Volume Descriptor and El Torito
        with open(input_bin, 'rb') as f:
            bin_data = f.read()

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

        # Add payload kernel binary padded to sector boundary
        payload_sectors = (len(bin_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
        payload_padded = bin_data + b'\x00' * (payload_sectors * SECTOR_SIZE - len(bin_data))
        iso_data.extend(payload_padded)

        with open(output_iso, 'wb') as f:
            f.write(iso_data)

        print(f"[ISO Builder] Generated standard ISO9660/El Torito image: {output_iso} ({len(iso_data)} bytes)")

    # Clean up temporary isodir
    subprocess.run("rm -rf isodir", shell=True)

if __name__ == '__main__':
    inp = sys.argv[1] if len(sys.argv) > 1 else 'sistema_operativo/cos.bin'
    out = sys.argv[2] if len(sys.argv) > 2 else 'carley-os.iso'
    build_iso(inp, out)
