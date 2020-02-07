BOOTBOOT Raspberry Pi 3 Implementation
======================================

See [BOOTBOOT Protocol](https://gitlab.com/bztsrc/bootboot) for common details.

On [Raspberry Pi 3](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bootmodes/sdcard.md) board the bootboot.img
is loaded from the boot (or firmware) partition on SD card as kernel8.img by start.elf. For separating firmware and boot
partitions see [documentation](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot_spec_1st_ed.pdf).

Machine state
-------------

In addition to standard mappings, the MMIO is also mapped in kernel space:

```
   -128M         MMIO      (0xFFFFFFFFF8000000)
```

Code is running in supervisor mode, at EL1 on all cores. Dummy exception handlers are installed, but your kernel should use
it's own handlers as soon as possible.

File system drivers
-------------------

For boot partition, RPi3 version expects FAT16 or FAT32 file systems (if the
initrd is a file and does not occupy the whole boot partition). The initrd can also be loaded over serial line,
running [raspbootcom](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/raspbootcom.c) on a remote machine.

Gzip compression is not recommended as reading from SD card is considerably faster than uncompressing.

Installation
------------

1. Copy __bootboot.img__ to **_FS0:\KERNEL8.IMG_**.

2. You'll need other [firmware files](https://gitlab.com/raspberrypi/firmware/tree/master/boot) as well (bootcode.bin, start.elf, fixup,dat).

3. If you have used a GPT disk with ESP as boot partition, then you need to map it in MBR so that Raspberry Pi
    firmware could find those files. The [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/mkboot.c)
    utility will do that for you.

Limitations
-----------

 - Initrd in ROM is not possible
 - Maps only the first 1G of RAM.
 - Cards other than SDHC Class 10 not supported.
 - Raspberry Pi does not have an on-board RTC, so always 0000-00-00 00:00:00 returned as bootboot.datetime.
 - Only supports SHA-XOR-CBC cipher, no AES
