#!/bin/bash
set -e
BASEDIR=$(cd `dirname $0` && pwd)
CC=${BASEDIR}/dependencies/out/path/bin/i686-elf-g++ # path to our cross-compiler build from my last blog post
AS=${BASEDIR}/dependencies/out/path/bin/i686-elf-as

echo "[ ] Install dependencies"
sudo apt -y install nasm xorriso qemu-system-i386

mkdir -p ${BASEDIR}/build
cd ${BASEDIR}/build

echo "[ ] Build boot code"
${AS} --32 ../boot/boot.s -o boot.o

echo "[ ] Compile Kernel"
${CC} -c ../kernel/kernel.cpp -o kernel.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti

echo "[ ] Link"
${CC} -T ../link/linker.ld -o codeshell.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

echo "[ ] Check is x86"
grub-file --is-x86-multiboot codeshell.bin
if [ $? -eq 0 ]; then
    echo "[ ] Output is x86 bootable"
else
    echo "[W] Output is not x86 bootable"
fi

echo "[ ] Creating bootable CD-Image with Grub-Bootloader"
mkdir -p isodir/boot/grub
cp codeshell.bin isodir/boot/codeshell.bin
cp ../grub/grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o codeshell.iso isodir

echo "[ ] Booting in QEmu"
qemu-system-i386 -cdrom codeshell.iso