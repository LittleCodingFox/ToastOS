#!/bin/sh

export BINDIR=bin

mkdir -p boot/boot/grub

echo -e "screen=800x600\nkernel=sys/core\n" > boot/BOOTBOOT/CONFIG || true

echo -e "menuentry \"BOOTBOOT test\" {\n  multiboot /BOOTBOOT/LOADER\n  module /BOOTBOOT/INITRD\n  module /BOOTBOOT/CONFIG\n  boot\n}" > boot/boot/grub/grub.cfg || true

grub-mkrescue -o $BINDIR/disk.iso boot

rm -Rf boot

#sh makevboxdisk.sh

echo Done!
