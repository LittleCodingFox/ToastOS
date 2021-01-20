#!/bin/sh

export BINDIR=bin
export OS_NAME=ToastOS

rm -Rf $BINDIR/*.img

dd if=/dev/zero of=$BINDIR/$OS_NAME.img bs=1M count=500

(echo g
echo n p
echo 1
echo 2048
echo +8M
echo t
echo 1
echo w) | fdisk -u -C500 -S63 -H16 $BINDIR/$OS_NAME.img

sudo mkdir -p /mnt/osdev

sudo umount /dev/loop0

sudo losetup -o1048576 /dev/loop0 $BINDIR/$OS_NAME.img

sudo mkfs.vfat -F16 -n "EFI System" /dev/loop0

sudo mount -tvfat /dev/loop0 /mnt/osdev

sudo cp -R boot/* /mnt/osdev

sudo umount /dev/loop0

sudo losetup -d /dev/loop0

echo Done!
