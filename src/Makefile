all:
	## bootloader ###
	nasm -f elf32 boot.s -o build/boot.o
	nasm -f elf32 gdt.s -o build/gdts.o
	### Kernel #####
	gcc -m32 -fno-stack-protector -fno-builtin -c kernel.c -o build/kernel.o
	gcc -m32 -fno-stack-protector -fno-builtin -c vga.c -o build/vga.o
	gcc -m32 -fno-stack-protector -fno-builtin -c gdt.c -o build/gdt.o
	### else #####
	ld -m elf_i386 -T linker.ld -o kernel build/boot.o build/kernel.o build/vga.o build/gdt.o build/gdts.o
	mv kernel codeshell/boot/kernel
	grub-mkrescue -o build/codeshell.iso codeshell/
	qemu-system-i386 build/codeshell.iso

clear:
	rm build/boot.o
	rm build/gdts.o
	rm build/kernel.o
	rm build/vga.o
	rm build/gdt.o
	rm build/codeshell.iso
