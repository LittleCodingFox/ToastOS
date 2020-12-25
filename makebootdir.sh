#!/bin/sh
BINDIR=bin
TMPDIR=tmp
KERNEL_NAME=kernel

mkdir -p boot/BOOTBOOT
mkdir -p boot/EFI/BOOT
mkdir -p $TMPDIR/sys

cp $BINDIR/$KERNEL_NAME.x86_64.elf $TMPDIR/sys/core

cd $TMPDIR
tar -czf ../INITRD *
cd ..

cp bootboot/dist/bootboot.efi boot/EFI/BOOT/BOOTX64.EFI
cp bootboot/dist/bootboot.bin boot/BOOTBOOT/LOADER
mv INITRD boot/BOOTBOOT/INITRD
