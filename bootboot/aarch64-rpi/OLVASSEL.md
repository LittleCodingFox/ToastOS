BOOTBOOT Raspberry Pi 3 Implementáció
=====================================

Általános leírásért lásd a [BOOTBOOT Protokoll](https://gitlab.com/bztsrc/bootboot)t.

A [Raspberry Pi 3](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bootmodes/sdcard.md) gépen a bootboot.img-t a
start.elf tölti be kernel8.ig néven az SD kártya első partíciójáról. Ha külön förmver és boot partíciót szeretnél, olvasd el
a [dokumentációt](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot_spec_1st_ed.pdf).

Gép állapot
-----------

A szokásos leképezéseken túl az MMIO is leképezésre kerül a kernel címtartományba:

```
   -128M         MMIO      (0xFFFFFFFFF8000000)
```

A kód felügyeleti módban, EL1-en fut minden processzoron. Egyszerű kivételkezelő be van állítva ugyan, de a kernelednek
minnél előbb érdemes sajátra váltania.

Fájl rendszer meghajtók
-----------------------

A boot partíció az RPi3 verzióban FAT16 vagy FAT32 fájl rendszer lehet (ha az initrd egy fájl és nem egy teljes partíció).
Az initrd soros vonalról is betölthető, ehhez a távoli gépen a [raspbootcom](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/raspbootcom.c)-ot kell futtatni.

A gzip tömörítés ellenjavalt, mivel az SD kártya sebessége lényegesen gyorsabb, mint a kitömörítésé.

Telepítés
---------

1. Másold be a __bootboot.img__-t az **_FS0:\KERNEL8.IMG_**-be.

2. Szükséged lesz egyéb [förmver fájlok](https://gitlab.com/raspberrypi/firmware/tree/master/boot)ra is (bootcode.bin, start.elf, fixup,dat).

3. Ha GPT-t használtál ESP-vel boot partíciónak, akkor le kell azt képezni az MBR-be, hogy a Raspberry Pi
    förmvere megtalálja azokat a fájlokat. A [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/mkboot.c)
    szerszám ezt megteszi neked.

Limitációk
----------

 - Nem lehet az initrd-t a ROM-ban tárolni
 - Csak az első 1G RAM-ot képezi le.
 - Egyéb kártyák, mint az SDHC Class 10 nincsenek támogatva.
 - Raspberry Pi-n nincs alaplapi RTC, ezért mindig 0000-00-00 00:00:00 kerül a bootboot.datetime-ba.
 - Csak a SHA-XOR-CBC titkosítást ismeri, nincs AES
