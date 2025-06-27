all:
	i686-elf-gcc --version
	### KERNEL ###
	echo "Compiling the kernel"
	gcc -m32 -fno-stack-protector  -fno-builtin -c kernel.c -o kernel.o
	gcc -m32 -fno-stack-protector  -fno-builtin -c vga.c -o vga.o
	gcc -m32 -fno-stack-protector  -fno-builtin -c keyboard.c -o keyboard.o
	
	### BOOTLOADER ###
	echo "Compiling the bootloader"
	nasm -f elf32 boot.s -o boot.o
	
	### LINKER ###
	echo "Linking all of the source into ISO"
	ld -m elf_i386 -T linker.ld -o kernel boot.o kernel.o vga.o keyboard.o
	mv kernel codeshell/boot/kernel
	grub-mkrescue -o codeshell.iso codeshell 
	qemu-system-x86_64 codeshell.iso
clear:
	echo "Clear the source directory"
	rm boot.o
	rm kernel.o
	rm codeshell/boot/kernel/kernel
	rm codeshell.iso
	rm vga.o
	rm keyboard.o
