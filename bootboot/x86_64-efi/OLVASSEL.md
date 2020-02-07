BOOTBOOT UEFI Implementáció
===========================

Általános leírásért lásd a [BOOTBOOT Protokoll](https://gitlab.com/bztsrc/bootboot)t.

Az [UEFI gépek](https://www.uefi.org/)en egy szabványos OS Loader alkalmazás használatos PCI Opció ROM-ként is.

Gép állapot
-----------

IRQ-k letiltva, GDT nincs meghatározva, de érvényes, IDT nincs beállítva. SSE, SMP engedélyezve. Kód felügyeleti módban, 0-ás gyűrűn
fut minden processzormagon.

Fájl rendszer meghajtók
-----------------------

Az UEFI verzióban a boot pratíción bármilyen fájl rendszer lehet, amit az EFI Simple File System Protocol támogat.
Ez az implementáció támogatja mind az SHA-XOR-CBC, mind az AES-256-CBC titkosítást.

Telepítés
---------

1. *UEFI lemez*: másold be a __bootboot.efi__-t az **_FS0:\EFI\BOOT\BOOTX64.EFI_**-be.

2. *UEFI ROM*: égesd ki a __bootboot.rom__-t, ami egy szabványos **_PCI Option ROM kép_**.

3. *GRUB*, *UEFI Boot Menedzser*: add hozzá a __bootboot.efi__-t az indítási opciókhoz.

Az EFI Shell-ből interaktívan is futtathatod a betöltőt paramétereket megadva a parancssorban.

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

Limitációk
----------

 - Az első 16G-nyi RAM-ot képezi le.
 - A PCI Option ROM-ot alá kell digitálisan írni ahhoz, hogy használni lehessen.
 - A tömörített initrd ROM 16M-nyi lehet.
