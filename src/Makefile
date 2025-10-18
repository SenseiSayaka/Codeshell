all:
	## bootloader ###
	nasm -f elf32 boot.s -o build/boot.o
	nasm -f elf32 gdt.s -o build/gdts.o
	nasm -f elf32 idt.s -o build/idt.o
	### Kernel #####
	gcc -m32 -fno-stack-protector -fno-builtin -c kernel.c -o build/kernel.o
	gcc -m32 -fno-stack-protector -fno-builtin -c vga.c -o build/vga.o
	gcc -m32 -fno-stack-protector -fno-builtin -c gdt.c -o build/gdt.o
	gcc -m32 -fno-stack-protector -fno-builtin -c idt.c -o build/idts.o
	gcc -m32 -fno-stack-protector -fno-builtin -c util.c -o build/util.o
	gcc -m32 -fno-stack-protector -fno-builtin -c timer.c -o build/timer.o
	gcc -m32 -fno-stack-protector -fno-builtin -c stdlib/stdio.c -o build/stdio.o
	gcc -m32 -fno-stack-protector -fno-builtin -c keyboard.c -o build/keyboard.o
	### else #####
	ld -m elf_i386 -T linker.ld -o kernel build/boot.o build/kernel.o build/vga.o build/gdt.o build/gdts.o build/util.o build/idt.o build/idts.o build/timer.o build/keyboard.o build/stdio.o
	mv kernel codeshell/boot/kernel
	grub-mkrescue -o build/codeshell.iso codeshell/
	qemu-system-i386 build/codeshell.iso

clear:
	rm build/boot.o
	rm build/gdts.o
	rm build/kernel.o
	rm build/vga.o
	rm build/gdt.o
	rm build/idt.o
	rm build/idts.o
	rm build/util.o
	rm build/timer.o
	rm build/keyboard.o
	rm build/stdio.o
	rm build/codeshell.iso
