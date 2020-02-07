BOOTBOOT BIOS / Multiboot / El Torito / Bővító ROM / Linux boot Implementáció
=============================================================================

Általános leírásért lásd a [BOOTBOOT Protokoll](https://gitlab.com/bztsrc/bootboot)t.

A [BIOS](http://www.scs.stanford.edu/05au-cs240c/lab/specsbbs101.pdf)-t támogató gépeken ugyanaz a betöltő működik
[Multiboot](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)tal, láncbetöltéssel MBR, VBR (GPT hibrid indítás)
és CDROM indító szektorból, vagy BIOS bővítő ROM-ból (szóval nemcsak a ramlemez lehet a ROM-ban, de maga a betöltő is), de
betölthető [Linux kernel](https://www.kernel.org/doc/html/latest/x86/boot.html)ként is (ISOLINUX, LoadLin stb.).

Gép állapot
-------------

IRQ-k letiltva, GDT nincs meghatározva, de érvényes, IDT nincs beállítva. SSE, SMP engedélyezve. Kód felügyeleti módban, 0-ás gyűrűn
fut minden processzormagon.

Telepítés
---------

1. *BIOS lemez / cdrom*: másold be a __bootboot.bin__-t az **_FS0:\BOOTBOOT\LOADER_**-be. Rakhatod az INITRD partíciódra de akár
        a partíciós területen kívülre is (lásd `dd conv=notrunc oseek=x`). Végezetül telepítsd a __boot.bin__-t az
        El Torito Boot katalógusába "no emulation" módban, vagy a Master Boot Record-ba (illetve Volume Boot Record-ba ha
        boot menedzsert használsz), a bootboot.bin első szektorának LBA címét lemetve 32 biten a 0x1B0 címre. A [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/mkboot.c)
        szerszám ezt megteszi neked.

2. *BIOS ROM*: égesd ki a __bootboot.bin__-t egy **_BIOS Bővító ROM_**-ba.

3. *GRUB*: add meg a __bootboot.bin__-t Multiboot "kernel"-nek a grub.cfg-ban, vagy lánctöltheted a __boot.bin__-t. Az initrd-t
és a környezeti fájl is betöltheted modulként (ebben a sorrendben). Ha nincs modul megadva, akkor a lemezen fogja keresni őket
a szokásos helyen. Példa:

```
menuentry "MyKernel" {
    multiboot /bootboot/loader      # bootboot.bin
    module /bootboot/initrd         # első modul az initrd (opcionális)
    module /bootboot/config         # második modul a konfigurációs fájl (opcionális)
    boot
}
```

Limitációk
----------

 - Mivel védett módban indul, csak az első 4G-nyi RAM-ot képezi le.
 - ROM-beli tömörített initrd maximum ~96k lehet.
 - A CMOS nvram nem tárol időzónát, ezért mindig GMT+0 kerül a bootboot.timezone-ba.
 - Csak a SHA-XOR-CBC titkosítást ismeri, nincs AES
