#!/bin/sh
BINDIR=bin
KERNEL_NAME=kernel

rm -Rf boot/

mkdir -p boot/EFI/BOOT
mkdir -p boot/boot

cp $BINDIR/$KERNEL_NAME boot/boot/
mv symbols.map boot/boot/
cp font.psf boot/boot/
cp -Rf toolchain/system-root/* ./dist
cd dist
tar -cvf ../boot/boot/initrd *
cd ..
