#!/bin/sh
BINDIR=bin
TMPDIR=tmp

mkdir -p boot/BOOTBOOT
mkdir -p boot/EFI/BOOT
mkdir -p $TMPDIR/sys

cp $BINDIR/mykernel.x86_64.elf $TMPDIR/sys/core

cd $TMPDIR
tar -czf ../INITRD *
cd ..

cp bootboot/dist/bootboot.efi boot/EFI/BOOT/BOOTX64.EFI
cp bootboot/dist/bootboot.bin boot/BOOTBOOT/LOADER
mv INITRD boot/BOOTBOOT/INITRD

