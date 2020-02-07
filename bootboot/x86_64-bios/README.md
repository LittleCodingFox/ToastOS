BOOTBOOT BIOS / Multiboot / El Torito / Expansion ROM / Linux boot Implementation
=================================================================================

See [BOOTBOOT Protocol](https://gitlab.com/bztsrc/bootboot) for common details.

On [BIOS](http://www.scs.stanford.edu/05au-cs240c/lab/specsbbs101.pdf) based systems, the same image can be loaded via
[Multiboot](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html),
chainload from MBR, VBR (GPT hybrid booting) or from CDROM boot record via __boot.bin__, run as a BIOS Expansion ROM
(so not only the ramdisk can be in ROM, but the loader as well), or can be loaded as a
[Linux kernel](https://www.kernel.org/doc/html/latest/x86/boot.html) (by ISOLINUX, LoadLin etc.).

Machine state
-------------

IRQs masked. GDT unspecified, but valid, IDT unset. SSE, SMP enabled. Code is running in supervisor mode in ring 0 on all cores.

Installation
------------

1. *BIOS disk / cdrom*: copy __bootboot.bin__ to **_FS0:\BOOTBOOT\LOADER_**. You can place it inside your INITRD partition
        or outside of partition area as well (with `dd conv=notrunc oseek=x`). Finally install __boot.bin__ in the
        El Torito Boot catalog with "no emulation" or in Master Boot Record (or in Volume Boot Record if you have a boot manager),
        saving bootboot.bin's first sector's LBA number in a dword at 0x1B0. The [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/mkboot.c)
        utility will do that for you.

2. *BIOS ROM*: install __bootboot.bin__ in a **_BIOS Expansion ROM_**.

3. *GRUB*: specify __bootboot.bin__ as a Multiboot "kernel" in grub.cfg, or you can also chainload __boot.bin__. You can load
the initrd and the environment file as modules (in this order). If no modules given, they will be loaded from disk as usual. Example:

```
menuentry "MyKernel" {
    multiboot /bootboot/loader      # bootboot.bin
    module /bootboot/initrd         # first module is the initrd (optional)
    module /bootboot/config         # second module is the environment file (optional)
    boot
}
```

Limitations
-----------

 - As it boots in protected mode, it only maps the first 4G of RAM.
 - Compressed initrd in ROM is limited to ~96k.
 - The CMOS nvram does not store timezone, so always GMT+0 returned in bootboot.timezone.
 - Only supports SHA-XOR-CBC, no AES
