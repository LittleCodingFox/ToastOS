#!/bin/sh

BINDIR=bin
OS_NAME=ToastOS
IMAGE_SIZE_MB=4096
LAST_SECTOR=$(((($IMAGE_SIZE_MB) - 1) * 1024 * 1024 / 512))

echo "Last sector: " $LAST_SECTOR

rm -Rf $BINDIR/*.img

dd if=/dev/zero of=$BINDIR/$OS_NAME.img bs=1M count=$IMAGE_SIZE_MB

(echo g
echo n
echo 1
echo 2048
echo $LAST_SECTOR
echo t
echo 1
echo p
echo w) | 
fdisk -u -C500 -S63 -H16 $BINDIR/$OS_NAME.img

git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1

make -C limine

./limine/limine-install $BINDIR/$OS_NAME.img

sudo mkdir -p /mnt/osdev

export LOOP=`sudo losetup -f`

#offset is 2048 * 512 (start of the partition)
sudo losetup -o1048576 $LOOP $BINDIR/$OS_NAME.img

sudo mkfs.vfat -F32 -n "EFI System" $LOOP

sudo mount -tvfat $LOOP /mnt/osdev

sudo cp limine_tar.cfg limine/limine.sys boot/

sudo mv boot/limine_tar.cfg boot/limine.cfg

sudo cp limine/BOOTX64.EFI boot/EFI/BOOT/

sudo cp -R boot/* /mnt/osdev

sync

rm -Rf limine

sudo umount $LOOP

sudo losetup -d $LOOP

echo Done!
