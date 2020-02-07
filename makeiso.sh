#!/bin/sh
export TMPDIR=tmp
export BINDIR=bin

mkdir -p $TMPDIR/sys
cp $BINDIR/mykernel.x86_64.elf $TMPDIR/sys/core
cd tmp
tar -czf ../INITRD *
cd ..

rm -Rf boot

rm -Rf $BINDIR/*.img
rm -Rf $BINDIR/*.iso
rm -Rf $BINDIR/*.cdr
rm -Rf $BINDIR/*.bin
rm -Rf $BINDIR/*.vdi

mkdir -p boot/EFI/BOOT
mkdir -p boot/BOOTBOOT

cp bootboot/bootboot.efi boot/EFI/BOOT/BOOTX64.EFI
cp bootboot/bootboot.bin boot/BOOTBOOT/LOADER
cp INITRD boot/BOOTBOOT/INITRD

hdiutil create -fs MS-DOS -layout NONE -sectors 2880 -srcfolder ./boot -format UDTO -o $BINDIR/disk

#IMAGE="$(echo $BINDIR/disk.cdr | sed -E 's/(\..{0,3})?\.cdr$/.img/')"
#mv "$BINDIR/disk.cdr" $IMAGE

#sh makevboxdisk.sh

echo Done!
