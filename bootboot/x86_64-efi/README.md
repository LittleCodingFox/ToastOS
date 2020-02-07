BOOTBOOT UEFI Implementation
============================

See [BOOTBOOT Protocol](https://gitlab.com/bztsrc/bootboot) for common details.

On [UEFI machines](http://www.uefi.org/), the PCI Option ROM is created from the standard EFI
OS loader application.

Machine state
-------------

IRQs masked. GDT unspecified, but valid, IDT unset. SSE, SMP enabled. Code is running in supervisor mode in ring 0 on all cores.

File system drivers
-------------------

For boot partition, UEFI version relies on any file system that's supported by EFI Simple File System Protocol.
This implementation supports both SHA-XOR-CBC and AES-256-CBC cipher.

Installation
------------

1. *UEFI disk*: copy __bootboot.efi__ to **_FS0:\EFI\BOOT\BOOTX64.EFI_**.

2. *UEFI ROM*: use __bootboot.rom__ which is a standard **_PCI Option ROM image_**.

3. *GRUB*, *UEFI Boot Manager*: add __bootboot.efi__ to boot options.

You can also run the loader in interactive mode from the EFI Shell, appending options to its command line.

```
FS0:\> EFI\BOOT\BOOTX64.EFI /?
BOOTBOOT LOADER (build Oct 11 2017)

SYNOPSIS
  BOOTBOOT.EFI [ -h | -? | /h | /? ] [ INITRDFILE [ ENVIRONMENTFILE [...] ] ]

DESCRIPTION
  Bootstraps an operating system via the BOOTBOOT Protocol.
  If arguments not given, defaults to
    FS0:\BOOTBOOT\INITRD   as ramdisk image and
    FS0:\BOOTBOOT\CONFIG   for boot environment.
  Additional "key=value" command line arguments will be appended to the
  environment. If INITRD not found, it will use the first bootable partition
  in GPT. If CONFIG not found, it will look for /sys/config inside the
  INITRD (or partition).

  As this is a loader, it is not supposed to return control to the shell.

FS0:\>
```

Limitations
-----------

 - Maps the first 16G of RAM.
 - PCI Option ROM should be signed in order to work.
 - Compressed initrd in ROM is limited to 16M.
