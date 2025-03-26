all:
	nasm main.asm
	qemu-system-i386 -fda main
