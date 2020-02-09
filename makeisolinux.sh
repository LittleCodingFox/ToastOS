#!/bin/sh

export BINDIR=bin

rm -Rf $BINDIR/*.img

dd if=/dev/zero of=$BINDIR/disk.img bs=1M count=500

(echo o
echo n
echo p
echo 1
echo 
echo 
echo t
echo b
echo a
echo p
echo w) | fdisk -u -C500 -S63 -H16 $BINDIR/disk.img

sudo mkdir -p /mnt/osdev

sudo umount /dev/loop0

sudo losetup -o1048576 /dev/loop0 $BINDIR/disk.img

sudo mkdosfs -F32 /dev/loop0

sudo mount -tvfat /dev/loop0 /mnt/osdev

sudo cp -R boot/* /mnt/osdev

sudo umount /dev/loop0

sudo losetup -d /dev/loop0

rm -Rf boot

sh makevboxdisk.sh

echo Done!
