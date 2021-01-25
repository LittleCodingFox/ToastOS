#!/bin/sh
BINDIR=bin
TMPDIR=tmp
KERNEL_NAME=kernel

rm -Rf boot/

mkdir -p boot/EFI/BOOT
mkdir -p $TMPDIR/sys

cd $TMPDIR
tar -czf ../INITRD *
cd ..

cp gnu-efi/x86_64/bootloader/main.efi boot/EFI/BOOT/BOOTX64.EFI
cp startup.nsh boot/
cp $BINDIR/$KERNEL_NAME.elf boot/
mv symbols.map boot/
cp font.psf boot/
