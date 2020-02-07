BOOTBOOT Reference Implementations
==================================

I provide pre-compiled images ready for use.

1. *x86_64-efi* the preferred way of booting on x86_64 architecture.
    Standard GNU toolchain and a few files from gnuefi (included).
    [bootboot.efi](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.efi) (94k), [bootboot.rom](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.rom) (94k)

2. *x86_64-bios* BIOS, Multiboot (GRUB), El Torito (CDROM), Expansion ROM and Linux boot compatible, OBSOLETE loader.
    If you want to recompile this, you'll need fasm (not included).
    [boot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/boot.bin) (512 bytes, works as MBR, VBR and CDROM boot record too), [bootboot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.bin) (11k, loaded by boot.bin, also BBS Expansion ROM and Multiboot compliant)

3. *aarch64-rpi* ARMv8 boot loader for Raspberry Pi 3
    [bootboot.img](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.img) (32k)

4. *mykernel* an example BOOTBOOT [compatible kernel](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel) in C which draws lines and boxes

Please note that the reference implementations do not support the full protocol at level 2,
they only handle static mappings which makes them level 1 loaders.

For quick test, you can find example bootable disk [images](https://gitlab.com/bztsrc/bootboot/tree/master/images) too.

BOOTBOOT Protocol
=================

Rationale
---------

The protocol describes how to boot an ELF64 or PE32+ executable inside an initial ram disk image
into clean 64 bit mode, without using any configuration or even knowing the file system of initrd.

On [BIOS](https://gitlab.com/bztsrc/bootboot/tree/master/x86_64-bios) based systems, the same image can be loaded via
Multiboot, chainload from MBR, VBR (GPT hybrid booting) and CDROM boot record, or run as a BIOS Expansion ROM
(so not only the ramdisk can be in ROM, but the loader as well).

On [UEFI machines](https://gitlab.com/bztsrc/bootboot/tree/master/x86_64-efi), it is a standard EFI OS Loader application.

On [Raspberry Pi 3](https://gitlab.com/bztsrc/bootboot/tree/master/aarch64-rpi) board the bootboot.img
is loaded from the boot partition on SD card as kernel8.img by start.elf.

The difference to other booting protocols is flexibility and portability;
only clean 64 bit support; and that BOOTBOOT expects the kernel to fit inside the
initial ramdisk. This is ideal for hobby OSes and micro-kernels. The advantage it gaves is that your kernel
can be splitted up into several files and yet they will be loaded together
as if it were a monolitic kernel. And you can use your own file system for the initrd.

Note: BOOTBOOT is not a boot manager, it's a boot loader protocol. If you want an interactive boot menu, you should
integrate that *before* a BOOTBOOT compatible loader is called. Like GRUB chainloading boot.bin (or loading bootboot.bin as a
multiboot "kernel" and initrd as a module) or adding bootboot.efi to UEFI Boot Manager's menu for example.

Licence
-------

The BOOTBOOT Protocol as well as the reference implementations are free
software and licensed under the terms of MIT licence.

```
    Copyright (C) 2017 bzt (bztsrc@gitlab)

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
```

Authors
-------

efirom: Michael Brown

zlib: Mark Adler

tinflate: Joergen Ibsen

raspbootcom: (GPL) Goswin von Brederlow

BOOTBOOT, FS/Z: bzt

Glossary
--------

* _boot partition_: the first bootable partition of the first bootable disk,
  a rather small one. Most likely an EFI System Partition with FAT, but can be
  any other partition as well if the partition is bootable (bit 2 set in flags).

* _environment file_: a maximum one page frame long, utf-8 [file](https://gitlab.com/bztsrc/bootboot/blob/master/README.md#environment-file) on boot partition
  at `BOOTBOOT\CONFIG` (or when your initrd is on the entire partition, `/sys/config`). It
  has "key=value" pairs (separated by newlines). The protocol
  only specifies two of the keys: "screen" for screen size,
  and "kernel" for the name of the executable inside the initrd.

* _initrd_: initial [ramdisk image](https://gitlab.com/bztsrc/bootboot/blob/master/README.md#installation)
  (probably in ROM or flash, or on a GPT boot partition at BOOTBOOT\INITRD, or it can occupy the whole partition, or can be loaded
  over the network or as a GRUB module). It's format and whereabouts are not specified (the good part :-) ) and can be optionally gzip compressed.

* _loader_: a native executable on the boot partition or in ROM. For multi-bootable disks
  more loader implementations can co-exists.

* _file system driver_: a separated function that parses initrd for the kernel file.
  Without one the first executable found will be loaded.

* _kernel file_: an ELF64 / PE32+ [executable inside initrd](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel),
  optionally with the following symbols: `mmio`, `fb`, `environment`, `bootboot` (see machine state and linker script).

* _BOOTBOOT structure_: an informational structure defined in [bootboot.h](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h).

Boot process
------------

1. the firmware locates the loader, loads it and passes control to it.
2. the loader initializes hardware (64 bit mode, screen resolution, memory map etc.)
3. then loads environment file and initrd file (probably from the boot partition or from ROM).
4. iterates on file system drivers, and loads kernel file from initrd.
5. if file system is not recognized, scans for the first executable in the initrd.
6. parses executable header and symbols to get link addresses (only level 2 compatible loaders).
7. maps linear framebuffer, environment and [bootboot structure](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h) accordingly.
8. sets up stack, registers and jumps to kernel entry point. See [example kernel](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel).

Machine state
-------------

When the kernel gains control, the memory mapping looks like this:

```
  -128M         "mmio" area           (0xFFFFFFFFF8000000)
   -64M         "fb" framebuffer      (0xFFFFFFFFFC000000)
    -2M         "bootboot" structure  (0xFFFFFFFFFFE00000)
    -2M+1page   "environment" string  (0xFFFFFFFFFFE01000)
    -2M+2page.. code segment   v      (0xFFFFFFFFFFE02000)
     ..0        stack          ^      (0x0000000000000000)
    0-16G       RAM identity mapped   (0x0000000400000000)
```

All information is passed at linker defined addresses. No API required at all, therefore the BOOTBOOT Protocol is
totally architecture and ABI agnostic. Level 1 expects these symbols at pre-defined addresses you see above, level 2
loaders parse the symbol table in executable to get the actual addresses.

The RAM (up to 16G) is identity mapped in the positive address range. Serial console is configured for 115200 baud,
8 data bits, no partity and 1 stop bit. Interrups are turned off and code is running in supervisor mode (ring 0 / EL1).

The screen is properly set up with a 32 bit packed pixel linear framebuffer, mapped at the negative address defined by
the `fb` symbol. Level 1 loaders limit the framebuffer size somewhere around 4096 x 4096 pixels (depends on scanline size
and aspect ratio too). That's more than enough for [Ultra HD 4K](https://en.wikipedia.org/wiki/4K_resolution)
(3840 x 2160). Level 2 loaders can place the fb anywhere in memory therefore they do not have such a limitation.

The main information [bootboot structure](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h) is mapped
at `bootboot` symbol. It consist of a fixed 128 bytes long header followed by various number of fixed
records. Your initrd (with the additional kernel modules and servers) is enitrely in the memory, and you can locate it
using this struct's *initrd_ptr* and *initrd_size* members. The physical address of the framebuffer can be found in
the *fb_ptr* field. The *boot time* and a platform independent *memory map* are also provided.

The configuration string (or command line if you like) is mapped at `environment` symbol.

Kernel's code segment is mapped at ELF header's `p_vaddr` or PE header's `code_base` (level 2 only). Level 1 loaders
load kernels at -2M, therefore limiting the kernel's size in 2M, including configuration, data, bss and stack. That
must be more than enough for all micro-kernels. Bss segment is after the text segment, growing upwards, and it's zerod-out
by the loader.

Co-processor enabled, and if Symmetric Multi Processing supported, all cores are running the same kernel code at once.

The stack is at the top of the memory, starting at zero and growing downwards. Each core has it's own 1k stack on SMP systems
(core 0's stack starts at 0, core 1's at -1024 etc.).

Environment file
----------------

Configuration is passed to your kernel as newline separated, zero terminated UTF-8 string with "key=value" pairs.

```
// BOOTBOOT Options

// --- Loader specific ---
// requested screen dimension. If not given, autodetected
screen=800x600
// elf or pe binary to load inside initrd
kernel=sys/core

// --- Kernel specific, you're choosing ---
anythingyouwant=somevalue
otherstuff=enabled
somestuff=100
someaddress=0xA0000
```

That cannot be larger than a page size (4096 bytes). Temporary variables will be appended at the end (from
UEFI command line). C style single line and multi line comments can be used. BOOTBOOT protocol only uses `screen` and
`kernel` keys, all the others and their values are up to your kernel (or drivers) to parse. Be creative :-)

To modify the environment when having booting issues, one will need to insert the disk into another machine (or boot a simple OS like DOS) and edit
BOOTBOOT\CONFIG on the boot partition. With UEFI, you can use the `edit` command provided by the EFI Shell or append
"key=value" pairs on the command line (value specified on command line takes precedence over the one in the file).

File system drivers
-------------------

The file system of the boot partition and how initrd is loaded from it is out of the scope of this specification:
the BOOTBOOT Protocol only states that a compatible loader must be able to load initrd and the environment file,
but does not describe how or from where. They can be loaded from nvram, ROM or over
network for example, it does not matter.

On the other hand BOOTBOOT does specify one API function to locate a file (the kernel)
inside the initrd image, but the ABI is also implementation (and architecture) specific.
This function receives a pointer to initrd in memory as well as the kernel's filename, and
returns a pointer to the first byte of the kernel and it's size. On error (if file system is
not recognized or the kernel file is not found) returns {NULL,0}. Plain simple.

```c
typedef struct {
    uint8_t *ptr;
    uint64_t size;
} file_t;

file_t myfs_initrd(uint8_t *initrd, char *filename);
```

The protocol expects that a BOOTBOOT compliant loader iterates on a list of drivers until one
returns a valid result. If all file system drivers returned {NULL,0}, the loader will brute-force
scan for the first ELF64 / PE32+ image in the initrd. This feature is quite comfortable when you
want to use your own file system but you don't have written an fs driver for it yet, or when your
"initrd" is a single, statically linked executable, like the Minix kernel. You just copy
your initrd on the boot partition, and you're ready to rock and roll!

The BOOTBOOT Protocol expects the file system drivers ([here](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-efi/fs.h),
[here](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/fs.inc) and [here](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/fs.h))
to be separated from the rest of the loader's source. This is so because it was designed to help the needs of hobby
OS developers, specially for those who want to write their own file systems.

The reference implementations support [cpio](https://en.wikipedia.org/wiki/Cpio) (all hpodc, newc and crc variants),
[ustar](https://en.wikipedia.org/wiki/Tar_(computing)), osdev.org's [SFS](http://wiki.osdev.org/SFS),
[James Molloy's initrd](http://www.jamesmolloy.co.uk/tutorial_html/8.-The%20VFS%20and%20the%20initrd.html)
format and OS/Z's native [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h) (with encryption support too).
Gzip compressed initrds also supported to save disk space and fasten up load time (not recommended on RPi3).

Example kernel
--------------

An [example kernel](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/kernel.c) is included with BOOTBOOT Protocol
to demostrate how to access the environment:

```c
#include <bootboot.h>

/* imported virtual addresses, see linker script below */
extern BOOTBOOT bootboot;           // see bootboot.h
extern unsigned char *environment;  // configuration, UTF-8 text key=value pairs
extern uint8_t fb;                  // linear framebuffer mapped

void _start()
{
    /*** NOTE: this code runs on all cores in parallel ***/
    int x, y, s=bootboot.fb_scanline, w=bootboot.fb_width, h=bootboot.fb_height;

    // cross-hair to see screen dimension detected correctly
    for(y=0;y<h;y++) { *((uint32_t*)(&fb + s*y + (w*2)))=0x00FFFFFF; }
    for(x=0;x<w;x++) { *((uint32_t*)(&fb + s*(h/2)+x*4))=0x00FFFFFF; }

    // red, green, blue boxes in order
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+20)*4))=0x00FF0000; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+50)*4))=0x0000FF00; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+80)*4))=0x000000FF; } }

    // say hello
    puts("Hello from a simple BOOTBOOT kernel");

    // hang for now
    while(1);
}
```

For compilation, see example bootboot kernel's [Makefile](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/Makefile) and
[link.ld](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/link.ld).

Because on an SMP system all cores executing the same code, you probably want to start your kernel with something like:

```c
if (currentcoreid == bootboot.bspid) {
    /* things to do on the bootstrap processor */
} else {
    /* things to do on the application processor(s) */
}
```

On x86_64, 'currentcoreid' is the Local Apic Id (cpuid[eax=1].ebx >> 24), on AArch64 that's (mpidr_el1 & 3).

Installation
------------

In the [images](https://gitlab.com/bztsrc/bootboot/tree/master/images) directory you can find test disk images and image creator
as well.

1. make an initrd with your kernel in it. Example:

```shell
mkdir -r tmp/sys
cp mykernel.x86_64.elf tmp/sys/core
# copy more files to tmp/ directory

# create your file system image or an archive. For example use one of these:
cd tmp
find . | cpio -H newc -o | gzip > ../INITRD
find . | cpio -H crc -o | gzip > ../INITRD
find . | cpio -H hpodc -o | gzip > ../INITRD
tar -czf ../INITRD *
mkfs ../INITRD .
```

2. Create FS0:\BOOTBOOT directory on the boot partition, and copy the image you've created
        into it. If you want, create a text file named CONFIG there too, and put your
        environment variables there. If you use a different name than "sys/core" for your
        kernel, specify "kernel=" in it.

Alternatively you can copy an uncompressed INITRD into the whole partition using your fs only, leaving FAT file system entirely out.
You can also create an Option ROM out of INITRD (on BIOS there's not much space ~64-96k, but on EFI it can be 16M).

3. copy the BOOTBOOT loader on the boot partition.

3.1. *UEFI disk*: copy __bootboot.efi__ to **_FS0:\EFI\BOOT\BOOTX64.EFI_**.

3.2. *BIOS disk*: copy __bootboot.bin__ to **_FS0:\BOOTBOOT\LOADER_**.

3.3. *Raspberry Pi 3*: copy __bootboot.img__ to **_FS0:\KERNEL8.IMG_**.

**IMPORTANT**: see the relevant port's README.md for more details.

Troubleshooting
---------------

```
BOOTBOOT-PANIC: LBA support not found
```

Really old hardware. Your BIOS does not support LBA. This message is generated by 1st stage loader (boot.bin).

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\LOADER not found
```

The loader (bootboot.bin) is not on the disk or it's starting LBA address is not recorded in the boot sector at dword [0x1B0]
(see [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/mkboot.c)). As the boot sector supports RAID mirror,
it will try to load the loader from other drives as well. This message is generated by 1st stage loader (boot.bin).

```
BOOTBOOT-PANIC: Hardware not supported
```

Really old hardware. On x86_64, your CPU is older than family 6.0 or PAE, MSR, LME features not supported.
On AArch64 it means the MMU does not support 4k granule size or at least 36 bit address size.


```
BOOTBOOT-PANIC: Unable to initialize SDHC card
```

The loader was unable to initialize EMMC for SDHC card access, probably hardware error or old card.

```
BOOTBOOT-PANIC: No GPT found
```

The loader was unable to load the GUID Partitioning Table.

```
BOOTBOOT-PANIC: No boot partition
```

There's no EFI System Partition nor any other bootable partition in the GPT. Or the FAT file system is found but corrupt
(contains inconsistent BPB data), or doesn't have a BOOTBOOT directory (with 8+3 MSDOS entry, not LFN).

```
BOOTBOOT-PANIC: Not 2048 sector aligned
```

This error is only shown by bootboot.bin (and not by bootboot.efi or bootboot.img) and only when
booted from CDROM in El Torito "no emulation" mode, and the boot partition file system's
root directory is not 2048 bytes aligned or the cluster size is not multiple of 2048
bytes. For FAT16 it depends on FAT table size and therefore on file system size. If you
see this message, increase the number of hidden sectors in BPB by 2. FAT32 file systems are
not affected.

```
BOOTBOOT-PANIC: Initrd not found
```

The loader could not find the initial ramdisk image on the boot partition.

```
BOOTBOOT-PANIC: Kernel not found in initrd
```

Kernel is not included in the initrd, or initrd's fileformat is not recognized by any of the file system
drivers and scanning haven't found a valid executable header in it.

```
BOOTBOOT-PANIC: Kernel is not a valid executable
```

The file that was specified as kernel could be loaded by fs drivers, but it's not an ELF64 or PE32+,
does not match the architecture, or does not have any program header with a loadable segment (p_vaddr or core_base)
in the negative range (see linker script). This error is also shown by level 2 loaders if the address of `mmio`, `fb`,
`bootboot` and `environment` symbols are not in the negative range.

```
BOOTBOOT-PANIC: GOP failed, no framebuffer
BOOTBOOT-PANIC: VESA VBE error, no framebuffer
BOOTBOOT-PANIC: VideoCore error, no framebuffer
```

The first part of the message varies on different platforms. It means that the loader was unable to set up linear
framebuffer with packed 32 bit pixels in the requested resolution. Possible solution is to modify screen to
`screen=800x600` or `screen=1024x768` in environment.

```
BOOTBOOT-PANIC: Unsupported cipher
```

This message is shown if the initrd is encrypted with a cipher that the loader does not support. Solution:
regenerate and encrypt the initrd image with SHA-XOR-CBC cipher, known to all the three implementations.
(Note: encryption is only supported for FS/Z initrd images.)

That's all, hope it will be useful!

Contributors
------------

I'd like to say special thanks to [Valentin Anger](https://gitlab.com/SyrupThinker) for throughfully testing this
code on many different real hardware.

