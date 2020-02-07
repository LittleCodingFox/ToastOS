BOOTBOOT Minta Bootolható Lemezkép Fájlok
=========================================

Általános leírásért lásd a [BOOTBOOT Protokoll](https://gitlab.com/bztsrc/bootboot)t.

- disk-rpi.img.gz: minta lemezkép AArch64-hez RaspberryPi 3-on
- disk-x86.img.gz: minta lemezkép x86_64-hez (CDROM, BIOS, UEFI)
- initrd.rom.gz: minta initrd ROM kép (beágyazott BIOS rendszerekhez)
- mkimg.c: egy nagyon szimpla és egyszerű lemezkép készítő

Mielőtt használhatnád a lemezképeket, ki kell csomagolni őket a `gzip -d` paranccsal.

Fordítás
--------

Nézz bele a Makefile-ba, az elején fogsz látni konfigurálható változókat.

- DISKSIZE: a teljes generálnadó lemezkép mérete megabájtban
- BOOTSIZE: a rendszerbetöltő partíció mérete kilobájtban (nem mega)
- PLATFORM: vagy "x86" vagy "rpi", ez választja ki, melyik lemezképet generálja

A `make all` parancsot futtatva a következő fájlokat hozza létre:

- initrd.bin: egy gzippelt hpodc cpio initrd kép, amiben egyelőre csak a futtatható kernel található
- bootpart.bin: a rendszerbetöltő partíció (ESP FAT16-al), ami tartalmazza az initrd-t és a betöltő programokat
- disk-(PLATFORM).img: hibrid lemezkép GPT partícióval

A disk-x86.img egy speciális hibrid lemezkép, amit átnevezhetsz disk-x86.iso-ra és kiégetheted egy CDROM-ra; vagy bebootolhatod
USB pendrávjról is BIOS valamint UEFI gépeken egyaránt.

A disk-rpi.img egy (Class 10) SD kártyára írható, és Raspberry Pi 3-on bootolható.

A lemezképekben mindössze egy boot partíció található. Az `fdisk` paranccsal szabadon hozzáadhatsz még partíciókat az izlésednek
megfelelően.

Tesztelés
---------

Hogy kipróbáld a BOOTBOOT-ot qemu-ban, használd a következő parancsokat:
```
make rom
```
Ez betölti a minta kernelt ROM-ból (lemez nélküli boot tesztelése BIOS Boot Spec alapján).
```
make bios
```
Ez betölti a minta kernelt lemezről (BIOS-al).
```
make cdrom
```
Ez El Torito "nem emulált" CDROM-ról tölti be a minta kernelt (BIOS-al).
```
make efi
```
Ez betölti a kernelt lemezről, UEFI használatával. Kell hozzá a TianoCode BIOS képfájl, amit a Makefile elején kell megadni.
```
make eficdrom
```
Ez betölti a kernelt CDROM-ról, UEFI használatával.
```
make grubcdrom
```
Ez grub-mkrescue hívásával hoz létre egy cdrom lemezképet, majd Multiboot-al betölti a BOOTBOOT-ot.
```
make linux
```
Ez betölti a minta kernelt úgy, hogy a BOOTBOOT-ot [Linux/x86 Boot Protocol](https://www.kernel.org/doc/html/latest/x86/boot.html)-al
indítja.
```
make sdcard
```
Ez "raspi3" gépet emulálva tölti be a minta kernelt SD kártya meghajtóról (kell hozzá a qemu-system-aarch64).
