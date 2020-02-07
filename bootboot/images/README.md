BOOTBOOT Example Bootable Disk Images
=====================================

See [BOOTBOOT Protocol](https://gitlab.com/bztsrc/bootboot) for common details.

- disk-rpi.img.gz: an example image for AArch64 and RaspberryPi 3
- disk-x86.img.gz: an example image for x86_64 (CDROM, BIOS, UEFI)
- initrd.rom.gz: an example initrd ROM image (for embedded BIOS systems)
- mkimg.c: is a very simple bootable disk image creator tool

Before you can use the images, uncompress them with `gzip -d`.

Compilation
-----------

Look at the beginning of the Makefile, you'll find configurable variables there.

- DISKSIZE: the overall disk image size to be generated in Mbytes
- BOOTSIZE: the boot partition size in Kbytes (not megabytes)
- PLATFORM: either "x86" or "rpi", this selects which disk image to create

Executing `make all` will create the following files:

- initrd.bin: a gzipped hpodc cpio initrd image, containing only the kernel executable for now
- bootpart.bin: the boot partition (ESP with FAT16), containing the initrd and the required files for booting
- disk-(PLATFORM).img: a hybrid disk image with GPT partitions

The disk-x86.img is a special hybrid image, which can be renamed to disk-x86.iso and then burnt to a CDROM; it can also be
booted from an USB stick in a BIOS machine as well as an UEFI machine.

The disk-rpi.img can be written to an SDCard (Class 10) and booted on a Raspberry Pi 3.

The disk images contain only one boot partition. Feel free to use `fdisk` and add more partitions to your needs.

Testing
-------

To test BOOTBOOT in qemu, you can use:
```
make rom
```
Will boot the example kernel from ROM (via BIOS Boot Spec, diskless test).
```
make bios
```
Will boot the example kernel from disk (using BIOS).
```
make cdrom
```
Will boot the example kernel in El Torito "no emulation" mode (BIOS).
```
make efi
```
Will boot the example kernel from disk using UEFI. You must provide your own TianoCore image, and set the path for it in the Makefile.
```
make eficdrom
```
Will boot the example kernel under UEFI from CDROM.
```
make grubcdrom
```
Will create a cdrom image using grub-mkrescue and boot BOOTBOOT using Multiboot.
```
make linux
```
Will boot the example kernel by booting BOOTBOOT via the [Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/latest/x86/boot.html).
```
make sdcard
```
Will boot the example kernel from SDCard emulating "raspi3" machine under qemu (requires qemu-system-aarch64).
