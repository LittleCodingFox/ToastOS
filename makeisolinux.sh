#!/bin/sh

export BINDIR=bin
export OS_NAME=ToastOS

rm -Rf $BINDIR/*.img

dd if=/dev/zero of=$BINDIR/$OS_NAME.img bs=1M count=500

(echo g
echo n p
echo 1
echo 2048
echo +100M
echo t
echo 1
echo n
echo 2
echo 206848
echo 1023966
echo p
echo w) | 
fdisk -u -C500 -S63 -H16 $BINDIR/$OS_NAME.img

git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1

make -C limine

./limine/limine-install $BINDIR/$OS_NAME.img

sudo mkdir -p /mnt/osdev

export LOOP=`sudo losetup -f`

sudo losetup -o1048576 $LOOP $BINDIR/$OS_NAME.img

sudo mkfs.vfat -F32 -n "EFI System" $LOOP

sudo mount -tvfat $LOOP /mnt/osdev

sudo cp -R boot/* /mnt/osdev

sudo mkdir -p /mnt/osdev/EFI/BOOT/

sudo cp limine.cfg limine/limine.sys /mnt/osdev

sudo cp limine/BOOTX64.EFI /mnt/osdev/EFI/BOOT/

sync

rm -Rf limine

sudo umount $LOOP

sudo losetup -d $LOOP

sudo losetup -o105906176 $LOOP $BINDIR/$OS_NAME.img

sudo mke2fs -b1024 $LOOP -L "Main Volume"

sudo mount -text2 $LOOP /mnt/osdev

sudo cp -R dist/* /mnt/osdev

sudo umount $LOOP

sudo losetup -d $LOOP

echo Done!
