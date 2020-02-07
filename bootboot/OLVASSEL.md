BOOTBOOT Referencia implementációk
==================================

Előre lefordított binárisok mellékelve, egyből használhatók.

1. *x86_64-efi* a preferált indítási mód x86_64-en.
    Szabvány GNU eszköztár plusz néhány fájl a gnuefi-ből (mellékelve).
    [bootboot.efi](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.efi) (94k), [bootboot.rom](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.rom) (94k)

2. *x86_64-bios* BIOS, Multiboot (GRUB), El Torito (CDROM), bővítő ROM és Linux boot kompatíbilis, RÉGI betöltő.
    Ha újra akarod fordítani, szükséged lesz a fasm-ra (nincs mellékelve).
    [boot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/boot.bin) (512 bájt, egyszerre MBR, VBR és CDROM indító szektor), [bootboot.bin](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.bin) (11k, a boot.bin tölti be, valamint BBS bővítő ROM és Multiboot kompatíbilis is)

3. *aarch64-rpi* ARMv8 betöltő Raspberry Pi 3-hoz
    [bootboot.img](https://gitlab.com/bztsrc/bootboot/raw/master/bootboot.img) (32k)

4. *mykernel* egy példa BOOTBOOT [kompatíbilis kernel](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel) C-ben írva, ami vonalakat húz meg színes dobozokat rajzol

Vedd figyelembe, hogy a referencia implementációk nem támogatják a teljes 2-es protokollt,
csak statikus memórialeképezéseket kezelnek, ami az 1-es protokoll szintnek felel meg.

Gyors kipróbáláshoz találsz bootolható képfájlokat az [images](https://gitlab.com/bztsrc/bootboot/tree/master/images) mappában.

BOOTBOOT Protokoll
==================

Célközönség
-----------

A protokoll definiálja, hogyan kell betölteni ELF64 vagy PE32+ futtathatókat egy induló ramlemezről tisztán
64 bites módban, mindenféle konfiguráció vagy akár a ramlemezkép formátumának ismerete nélkül.

A [BIOS](https://gitlab.com/bztsrc/bootboot/tree/master/x86_64-bios)-t támogató gépeken ugyanaz a betöltő működik
Multiboottal, láncbetöltéssel MBR, VBR (GPT hibrid indítás) és CDROM indító szektorból, vagy BIOS bővítő ROM-ból
(szóval nemcsak a ramlemez lehet a ROM-ban, de maga a betöltő is).

Az [UEFI gépek](https://gitlab.com/bztsrc/bootboot/tree/master/x86_64-efi)en egy szabványos OS Loader alkalmazás.

A [Raspberry Pi 3](https://gitlab.com/bztsrc/bootboot/tree/master/aarch64-rpi) gépen a bootboot.img-t a start.elf
tölti be kernel8.img néven az SD kártya első partíciójáról.

A különbség más betöltő protokollokhoz képest a rugalmasság és a hordozhatóság; a tisztán 64 bit támogatás; és hogy
a BOOTBOOT a kernel-t a ramlemezképből tölti be. Ez ideális hobbi OS-ek és mikrokernelek számára. Ez biztosítja, hogy
a kerneled felbontható több fájlra, mégis megadja azt az előnyt, hogy egyszerre tölti be mind, mintha egy monolitikus
kernel lenne. Ráadásul mindehhez a saját fájl rendszeredet is használhatod.

Megjegyzés: a BOOTBOOT nem egy boot menedzser, hanem egy boot protokoll. Ha interaktív indítómenüt szeretnél, akkor
azt a BOOTBOOT kompatíbilis betöltő *elé* kell integrálnod. Például a GRUB lánctöltheti a boot.bin-t (vagy Multiboot
"kernel"-ként a bootboot.bin-t és modulként a ramlemezt) vagy a bootboot.efi hozzáadható az UEFI Boot menedzser menüjéhez.

Licensz
-------

A BOOTBOOT Protokoll és a referencia implementációk mind szabad szoftverek és az MIT licensz feltételei szerint kerülnek
terjesztésre.

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

Szerzők
-------

efirom: Michael Brown

zlib: Mark Adler

tinflate: Joergen Ibsen

raspbootcom: (GPL) Goswin von Brederlow

BOOTBOOT, FS/Z: bzt

Kifejezések
-----------

* _boot partíció_: az első indítható lemez első indítható partíciója, lehetőleg egy kissebb.
  Nagy valószínűséggel egy EFI Rendszer Partció FAT fájl rendszerrel, de lehet akármi más is,
  ha az adott partíció indítható (az attribútumoknál a 2. bit be van állítva).

* _környezeti fájl_: legfeljebb egy lap méretű, utf-8 [fájl](https://gitlab.com/bztsrc/bootboot/blob/master/README.md#environment-file) a boot partíción
  `BOOTBOOT\CONFIG` néven (vagy ha a ramlemez az egész partíciót elfoglalja, akkor `/sys/config`). Egyszerű
  "kulcs=érték" párokat tartalmaz (újsor karakterrel elválasztva). A protokoll mindössze két
  kulcsot definiál: "screen" a képernyő méretét adja meg, a "kernel" pedig a futtatható fájl nevét a ramlemezen.

* _initrd_: induló [ramlemez kép](https://gitlab.com/bztsrc/bootboot/blob/master/README.md#installation)
  (lehet ROM-ban vagy flash-en, vagy egy GPT boot partíción BOOTBOOT\INITRD néven, vagy kitöltheti az egész partíciót, de akár
  hálózatról vagy GRUB modulként is betöltehető). A formátuma és a holléte nincs meghatározva (a jó rész :-) ) és gzip tömörítést
  is használhat.

* _betöltő_: (loader) egy natív futtatható a boot partíción vagy ROM-ban. Többarchitektúrás lemezeken több
  betöltő implementáció is lehet egyszerre.

* _fájl rendszer meghajtó_: egy elkölönített funkció, ami az initrd-ben keresi meg a kernelt.
  Ha nincs ilyen, akkor a legelső futtathatót tölti be az initrd-ről.

* _kernel fájl_: egy ELF64 / PE32+ [futtatható az initrd-ben](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel),
  opcionálisan a következő szimbólumokkal: `mmio`, `fb`, `environment`, `bootboot` (lásd gép állapot és linker szkript alább).

* _BOOTBOOT struktúra_: a [bootboot.h](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h)-ban definiált struktúra.

Betöltés menete
---------------

1. a förmver megkeresi a betöltőt, betölti és elindítja.
2. a betöltő inicializálja a hardvert (64 bites mód, képernyőfelbontás, memória térkép stb.)
3. aztán betölti a környezeti fájlt és az initrd-t (valószínűleg a boot partícióról vagy ROM-ból).
4. sorra meghívja a fájl rendszer meghajtókat, hogy betöltse a kernelt az initrd-ről.
5. ha egyik meghajtó sem járt szerencsével, akkor megkeresi az első futtathatót az initrd-ben.
6. értelmezi a futtatható fejlécét és a szimbólumokat, hogy megtalálja a címeket (csak 2-es szintű betöltők).
7. ennek megfelelően leképezi a framebuffert, környezetet és a [bootboot struktúrá](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h)t.
8. beállítja a vermet, regisztereket és a kernel belépési pontjára ugrik. Lásd [példa kernel](https://gitlab.com/bztsrc/bootboot/tree/master/mykernel).

Gép állapot
-----------

Amikor a kernel átveszi a vezérlést, a memória a következőket tartalmazza:

```
  -128M         "mmio" terület             (0xFFFFFFFFF8000000)
   -64M         "fb" framebuffer           (0xFFFFFFFFFC000000)
    -2M         "bootboot" struktúra       (0xFFFFFFFFFFE00000)
    -2M+1page   "environment" sztring      (0xFFFFFFFFFFE01000)
    -2M+2page.. kód szegmens   v           (0xFFFFFFFFFFE02000)
     ..0        verem          ^           (0x0000000000000000)
    0-16G       egy-az-egy leképzett RAM   (0x0000000400000000)
```

Minden információ a linker által meghatározott címen kerül átadásra. Nincs szükség API-ra, ezért a BOOTBOOT Protokoll
teljesen architektúra és ABI független. Az 1-es szintet támogató betöltők a fent látható fix címeket használják, míg
a 2-es szintű betöltők a futtatható által definiált szimbólumokat használva állapítják meg a címeket.

A RAM (egészen 16G-ig) egy-az-egyben le van képezve a pozitív címtartományban. A soros vonali konzol 115200 baudra,
8 adatbittel, paritás nélkül és 1 stopbitre van felkonfigurálva. Megszakítások le vannak tiltva, a kód pedig felügyeleti
szinten (0-ás gyűrű / EL1) fut.

A képernyű megfelelő felbontásra van állítva 32 bites picelekkel, az `fb` szimbólum által jelölt, negatív címre leképezve.
Az 1-es szintű betöltők korlátozzák a képernyő méretét valahol 4096 x 4096 pixelnél (függ a szkensortól és a képaránytól is).
Ez több, mint elég az [Ultra HD 4K](https://en.wikipedia.org/wiki/4K_resolution) (3840 x 2160) felbontáshoz. A 2-es szintű
betöltők akárhoz képesek kezelni az fb szimbólumot, ezért rájuk nem vonatkozik ez a megszorítás.

A fő információs [bootboot struktúra](https://gitlab.com/bztsrc/bootboot/blob/master/bootboot.h) a `bootboot` szimbólumra
van leképezve. Ez áll egy fix 128 bájtos fejlécből, amit egy változó hosszúságú, de fix méretű rekordok követnek. Az initrd
(benne a további kernel modulokkal és szerverekkel) teljes egészében a memóriában van, és ezen struktúra *initrd_ptr* és
*initrd_size* mezői mutatnak rá. A framebuffer fizikai címe az *fb_ptr* mezőben található. Az *indulási rendszer idő* és
a platform független *memóriatérkép* szintén itt található.

A konfigurációs sztring (vagy parancssor, ha úgy tetszik) az `environment` szimbólumra van leképezve.

A kernel kód szegmense az ELF fejléc `p_vaddr` vagy a PE fejléc `code_base` mezői alapján kerülnek leképezésre (csak 2-es
szint). Az 1-es szintű betöltők mindig -2M-re töltik a kernelt, ezzel limitálva a teljes méretét 2M-re, beleértve a
konfigurációt, adatot, inicializálatlan adatterületet és a vermet. Ennek több, mint elégnek kell lennie bármilyen mikrokernel
számára. Az inicialiyálatlan adatterület a text szegmens után jön, felfelé növekszik, és a betöltő kinullázza.

A társprocesszor (lebegőpontos számtás) be van kapcsolva, és ha a Szimmetrikus Többmagos Feldolgozás (SMP) támogatott, akkor
minden CPU mag ugyanazt a kernel kódot hajtja végre egyszerre.

A verem a memória tetején található, 0-tól kezdve és lefelé növekszik. Minden magnak saját 1k-s verme van SMP rendszereken
(a 0-ás mag verme kezdődik 0-án, az 1-es magé -1024-nél stb.)

Környezeti fájl
----------------

A konfiguráció egy sorvége karakterekkel elhatárolt, nullával lezárt, "kulcs=érték" UTF-8 sztringként kerül átadásra a kernelnek.

```
// BOOTBOOT Opciók

// --- betöltő specifikus ---
// igényelt képernyő felbontás. ha nincs megadva, akkor detektált
screen=800x600
// elf vagy pe bináris az initrd-ben
kernel=sys/core

// --- kernel specifikus, ami csak tetszik ---
amitcsakakartok=érték
megintmas=be
valamiertek=100
valamicim=0xA0000
```

Ez nem lehet nagyobb a lapméretnél (4096 bájt). Az ideiglenes változók a végére kerülnek (az UEFI parancssorból).
C stílusű egy- és többsoros kommentek használhatók. A BOOTBOOT Protokoll csak a `screen` és a `kernel` kulcsokat
használja, az összes többi és az értékük formátuma a kerneleden (vagy az eszközmeghajtóidon) múlik. Légy kreatív :-)

Ha indítási probléma van, a konfiguráció módosításához át kell rakni a lemezt egy másik gépbe (vagy egy egyszerű oprendszert
indítani, mint a DOS) és megszerkeszteni a BOOTBOOT\CONFIG fájlt a boot partíción. UEFI alatt használható az EFI Shell által
nyújtott `edit` parancs, vagy a "kulcs=érték" párok a parancssorban is megadhatók (a parancsori kulcsok felülbírálják a fájlban
megadottakat).

Fájl rendszer meghajtók
-----------------------

A boot partíció fájl rendszer formátuma kívül esik a specifikáción: a BOOTBOOT Protokoll csak annyit vár el,
hogy a kompatíbilis betöltő valahonnan be tudja tölteni az initrd-t és a környezeti fájlt, de nem szabja meg, hogy
honnan és hogy hogyan. Lehet nvram-ban, ROM-ban, jöhet hálózatról például, nem számít.

Más részről az initrd ramlemezre vonatkozóan a BOOTBOOT definiál pontosan egy függvényt a kernel fájl megkeresésére,
de az ABI-t nem írja elő (ami architektúra specifikus úgyis). Ez a funkció kap egy mutatót a ramlemezre, és egyet a kernel
fájl nevére, és visszaadja a fájl első adatbájtjára a mutatót valamint a fájl méretét. Ha hiba volt (a fájl rendszert nem
ismerte fel, vagy a megadott nevű fájlt nem találta) akkor {NULL, 0}-t ad vissza. Ilyen egyszerű.

```c
typedef struct {
    uint8_t *ptr;
    uint64_t size;
} file_t;

file_t myfs_initrd(uint8_t *initrd, char *filename);
```

A protokoll elvárja, hogy a BOOTBOOT kompatíbilis betöltők végigmenjenek egy meghajtó listán, amíg valamelyik
érvényes választ nem ad. Ha mindegyik {NULL,0}-al tért vissza, akkor a betöltő bájtról bájtra megkeresi a legelső
ELF64 / PE32+ binárist az initrd-ben. Ez roppant hasznos, ha egyedi fájl rednszerünk van és még nem írtunk hozzá saját
meghajtót, vagy amikor az "initrd" egy statikusan linkelt futtatható, mint például a Minix. Csak bemásolod az initrd-t
a boot partícióra és már mehet is!

A BOOTBOOT Protokoll előírja, hogy a fájl rendszer meghajtók ([itt](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-efi/fs.h),
[itt](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/fs.inc) és [itt](https://gitlab.com/bztsrc/bootboot/blob/master/aarch64-rpi/fs.h))
elkülönítve legyenek a betöltő forrásának többi részétől. Ez azért van, mert elsősorban azoknak a hobbi OS fejlesztőknek
készült, akik saját fájl rendszert használnak.

A referencia implementációk támogatják a [cpio](https://en.wikipedia.org/wiki/Cpio)-t (hpodc, newc és crc variáns),
az [ustar](https://en.wikipedia.org/wiki/Tar_(computing))-t, az osdev.org féle [SFS](http://wiki.osdev.org/SFS)-t,
[James Molloy initrd formátumá](http://www.jamesmolloy.co.uk/tutorial_html/8.-The%20VFS%20and%20the%20initrd.html)t
valamint az OS/Z natív [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h)-jét (utóbbit titkosítással is).
Az initrd-k gzippel tömörítettek is lehetnek, hogy kissebb lemezterületet foglaljanak (nem javasolt RPI3-on).

Példa kernel
------------

Egy [példa kernel](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/kernel.c) is lett mellékelve a BOOTBOOT Protokollhoz,
hogy bemutassa, miként lehet elérni a betöltő által nyújtott információkat:

```c
#include <bootboot.h>

/* importált virtuális címek, lásd linker szkript */
extern BOOTBOOT bootboot;           // lásd bootboot.h
extern unsigned char *environment;  // konfiguráció, UTF-8 szöveg kulcs=érték párokkal
extern uint8_t fb;                  // lineáris framebuffer leképezés

void _start()
{
    /*** FIGYELEM: ez a kód párhuzamosan fut minden processzormagon ***/
    int x, y, s=bootboot.fb_scanline, w=bootboot.fb_width, h=bootboot.fb_height;

    // célkereszt, hogy lássuk, a felbontás jó-e
    for(y=0;y<h;y++) { *((uint32_t*)(&fb + s*y + (w*2)))=0x00FFFFFF; }
    for(x=0;x<w;x++) { *((uint32_t*)(&fb + s*(h/2)+x*4))=0x00FFFFFF; }

    // piros, zöld és kék dobozok, ebben a sorrendben
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+20)*4))=0x00FF0000; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+50)*4))=0x0000FF00; } }
    for(y=0;y<20;y++) { for(x=0;x<20;x++) { *((uint32_t*)(&fb + s*(y+20) + (x+80)*4))=0x000000FF; } }

    // köszönünk
    puts("Hello from a simple BOOTBOOT kernel");

    // leállunk egyelőre
    while(1);
}
```

A fordításhoz lásd a mellékelt példa kernel [Makefile](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/Makefile)-ját és
[link.ld](https://gitlab.com/bztsrc/bootboot/blob/master/mykernel/link.ld)-jét.

Mivel SMP rendszeren minden processzormag ezt a kódot futtatja, ezért valósznűleg valami ilyesmivel fogod kezdeni a kerneled:

```c
if (currentcoreid == bootboot.bspid) {
    /* a bootstrap processzoron futtatandó dolgok */
} else {
    /* alkalmazás processzor(ok)on futtatandó dolgok */
}
```

Az x86_64-on a 'currentcoreid' a Local Apic Id (cpuid[eax=1].ebx >> 24), míg AArch64-on ez az (mpidr_el1 & 3).

Telepítés
---------

Az [images](https://gitlab.com/bztsrc/bootboot/tree/master/images) mappában találsz teszt képfájlokat és képfájl készítőt is.

1. Hozz létre egy initrd-t benne a kerneleddel. Példák:

```shell
mkdir -r tmp/sys
cp mykernel.x86_64.elf tmp/sys/core
# még több fájl másolása a tmp mappába

# lemezkép vagy csomagolt fájl létrehozása, pl használd ezek közül az egyik parancsot:
cd tmp
find . | cpio -H newc -o | gzip > ../INITRD
find . | cpio -H crc -o | gzip > ../INITRD
find . | cpio -H hpodc -o | gzip > ../INITRD
tar -czf ../INITRD *
mkfs ../INITRD .
```

2. Hozd létre az FS0:\BOOTBOOT könyvtárat a boot partíción, és másold be az előbb létrehozott initrd-t.
        Ha akarsz, csinálhatsz egy CONFIG nevű szöveges fájlt is ide, és írd bele a konfigurációdat.
        Ha nem a "sys/core" néven található a kerneled, akkor add meg a "kernel=" változóban itt.

Alternatívaként a tömörítetlen INITRD-t bemásolhatod az egész partícióra, csak a saját fájl rendszeredet használva, a FAT
fájl rednszert teljes egészében kihagyva. Csinálhatsz belőle egy bővító ROM-ot is (BIOS-on nincs sok hely, ~64-96k, de EFI-n
elmehetsz egészen 16M-ig).

3. másold be a BOOTBOOT betöltőt a boot partícióra.

3.1. *UEFI lemez*: másold a __bootboot.efi__-t az **_FS0:\EFI\BOOT\BOOTX64.EFI_**-be.

3.2. *BIOS lemez*: másold a __bootboot.bin__-t az **_FS0:\BOOTBOOT\LOADER_**-be.

3.3. *Raspberry Pi 3*: másold a __bootboot.img__-t az **_FS0:\KERNEL8.IMG_**-be.

**FONTOS**: olvasd el a kérdéses implementáció README.md-jét is.

Hibakeresés
-----------

```
BOOTBOOT-PANIC: LBA support not found
```

Nagyon régi hardver. A BIOSod nem támogatja az LBA-t. Ezt az üzenetet az első betöltő szektor (boot.bin) írja ki.

```
BOOTBOOT-PANIC: FS0:\BOOTBOOT\LOADER not found
```

A fő betöltő (bootboot.bin) nem található a lemezen, vagy az induló szektorcíme nincs jól rögzítve az indítószektor 32 bites
[0x1B0] címén (lásd [mkboot](https://gitlab.com/bztsrc/bootboot/blob/master/x86_64-bios/mkboot.c)). Mivel a betöltő szektor
támogatja a RAID tükröket, több meghajtóról is meg fogja próbálni betölteni a betöltőt. Ezt az üzenetet az első betöltő szektor
(boot.bin) írja ki.

```
BOOTBOOT-PANIC: Hardware not supported
```

Nagyon régi hardver. Az x86_64-en azt jelenti, a CPU család régebbi, mint 6.0, illetve nincs a PAE, az MSR vagy az LME támogatva.
AArch64-en azt jelenti, hogy az MMU nem kezeli a 4k-s lapméretet vagy legalább 36 bites címteret.


```
BOOTBOOT-PANIC: Unable to initialize SDHC card
```

A betöltő nem tudta inicializálni az EMMC-t SD kártya olvasáshoz, valószínáleg hardver hiba vagy régi kártya.

```
BOOTBOOT-PANIC: No GPT found
```

Nem található a lemezen GUID Partíciós Tábla.

```
BOOTBOOT-PANIC: No boot partition
```

Vagy a lemezen nincs GPT, vagy nincs benne EFI Rendszer partíció se egyéb indítható partíció. Vagy az azon lévő FAT
fájl rendszer sérült, illetve nincs benne BOOTBOOT könyvtár.

```
BOOTBOOT-PANIC: Not 2048 sector aligned
```

Ezt a hibát csak a bootboot.bin írja ki (és nem a bootboot.efi vagy a bootboot.img), és csakis akkor,
ha CDROM-ról El Torito "no emulation" módban indult, és a boot partíció fájl rendszerének főkönyvtára
nincs 2048 bájtra igazítva, vagy a kluszterméret nem 2048 bájt többszöröse. FAT16 esetén ez a FAT tábla
méretén és emiatt a fájl rendszer méretén múlik. Ha ezt látod, akkor adj hozzá 2 rejtett szektort a BPB-ben.
FAT32 fájl rendszer esetén nem fordulhat elő.

```
BOOTBOOT-PANIC: Initrd not found
```

A betöltő nem találta az initrd-t a boot partíción.

```
BOOTBOOT-PANIC: Kernel not found in initrd
```

A megadott kernel nem található az initrd-n, vagy az initrd formátuma nem ismert és a keresés nem talált
egyáltalán érvényes futtathatót benne.

```
BOOTBOOT-PANIC: Kernel is not a valid executable
```

A megadott kernel fájlt megtalálta ugyan az initrd-n valamelyik fájl rendszer meghajtó, de az nem ELF64 se PE32+ formátumú,
vagy nem az adott architáktúrára van fordítva, vagy nincs benne betölthető szegmens definíció a negatív címtartományban
megadva (lásd linker szkript). Ezt a hibát a 2-es szintű betöltők is kiírhatják, ha az `mmio`, `fb`, `bootboot` vagy
`environment` szimbólumok címei nincsenek a negatív címtartományban.

```
BOOTBOOT-PANIC: GOP failed, no framebuffer
BOOTBOOT-PANIC: VESA VBE error, no framebuffer
BOOTBOOT-PANIC: VideoCore error, no framebuffer
```

Az üzenet első fele platformonként változik. Azt jelenti, hogy a betöltő nem tudta beállítani a lineáris framebufffert
32 bites tömörített pixelekkel a kért felbontásban. Lehetséges megoldás a felbontás módosítása `screen=800x600`-ra vagy
`screen=1024x768`-ra a környezeti fájlban.

```
BOOTBOOT-PANIC: Unsupported cipher
```

Ezt akkor írja ki, ha az initrd olyan titkosítást használ, amit a betöltő nem ismer. Megoldás: újra kell generálni az
initrd-t SHA-XOR-CBC-vel, amit minden betöltő implementációnak támogatnia kell (megjegyzés: a titikosítás csak FS/Z
esetén támogatott).

Ez minden, remélem hasznososnak találod!

Hozzájárulások
--------------

Szeretnék külön köszönetet mondani [Valentin Anger](https://gitlab.com/SyrupThinker)-nek, amiért alaposan letesztelte a
kódot számtalan igazi vason is.

