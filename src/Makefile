all:
	## bootloader ###
	nasm -f elf32 boot.s -o build/boot.o
	nasm -f elf32 gdt.s -o build/gdts.o
	nasm -f elf32 idt.s -o build/idt.o
	nasm -f elf32 task.s -o build/task.o
	### Kernel #####
	gcc -m32 -fno-stack-protector -fno-builtin -c kernel.c  -o build/kernel.o
	gcc -m32 -fno-stack-protector -fno-builtin -c vga.c     -o build/vga.o
	gcc -m32 -fno-stack-protector -fno-builtin -c gdt.c     -o build/gdt.o
	gcc -m32 -fno-stack-protector -fno-builtin -c idt.c     -o build/idts.o
	gcc -m32 -fno-stack-protector -fno-builtin -c util.c    -o build/util.o
	gcc -m32 -fno-stack-protector -fno-builtin -c timer.c   -o build/timer.o
	gcc -m32 -fno-stack-protector -fno-builtin -c stdlib/stdio.c -o build/stdio.o
	gcc -m32 -fno-stack-protector -fno-builtin -c keyboard.c -o build/keyboard.o
	gcc -m32 -fno-stack-protector -fno-builtin -c shell.c   -o build/shell.o
	gcc -m32 -fno-stack-protector -fno-builtin -c kmalloc.c -o build/kmalloc.o
	gcc -m32 -fno-stack-protector -fno-builtin -c fat12.c   -o build/fat12.o
	gcc -m32 -fno-stack-protector -fno-builtin -c task.c    -o build/taskc.o
	gcc -m32 -fno-stack-protector -fno-builtin -c pmm.c     -o build/pmm.o
	gcc -m32 -fno-stack-protector -fno-builtin -c paging.c  -o build/paging.o
	gcc -m32 -fno-stack-protector -fno-builtin -c syscall.c -o build/syscall.o
	gcc -m32 -fno-stack-protector -fno-builtin -c elf.c     -o build/elf.o
	gcc -m32 -fno-stack-protector -fno-builtin -c ata.c     -o build/ata.o
	gcc -m32 -fno-stack-protector -fno-builtin -c fat12_ata.c -o build/fat12_ata.o
	gcc -m32 -fno-stack-protector -fno-builtin -c vga_font.c -o build/vga_font.o
	gcc -m32 -fno-stack-protector -fno-builtin -c vga_font_data.c -o build/vga_font_data.o
	### user programs ###
	nasm -f elf32 programs/crt0.s -o build/crt0.o
	gcc -m32 -fno-stack-protector -fno-builtin -nostdlib -nostdinc \
	    -ffreestanding -O2 -c programs/hello.c -o build/hello.o
	ld -m elf_i386 -T programs/linker.ld -o build/hello.elf \
	    build/crt0.o build/hello.o
	### filesystem image ###
	dd if=/dev/zero of=build/fs.img bs=512 count=2880
	mkfs.fat -F 12 build/fs.img
	mcopy -i build/fs.img fs/motd.txt  ::MOTD.TXT
	mcopy -i build/fs.img build/hello.elf ::HELLO.ELF
	cp build/fs.img codeshell/boot/fs.img
	### link and run ###
	ld -m elf_i386 -T linker.ld -o kernel \
	    build/boot.o build/kernel.o build/vga.o build/gdt.o build/gdts.o \
	    build/util.o build/idt.o build/idts.o build/timer.o build/keyboard.o \
	    build/stdio.o build/shell.o build/kmalloc.o build/fat12.o \
	    build/task.o build/taskc.o build/pmm.o build/paging.o build/syscall.o build/elf.o build/ata.o \
	    build/fat12_ata.o build/vga_font.o build/vga_font_data.o
	mv kernel codeshell/boot/kernel
	### ATA disk image ###
	[ -f build/disk.img ] || ( \
	    dd if=/dev/zero of=build/disk.img bs=512 count=204800 && \
	    mkfs.fat -F 12 build/disk.img && \
	    mcopy -i build/disk.img fs/motd.txt   ::MOTD.TXT && \
	    mcopy -i build/disk.img build/hello.elf ::HELLO.ELF \
	)
	### run ###
	grub-mkrescue -o build/codeshell.iso codeshell/
	qemu-system-i386 build/codeshell.iso -drive file=build/disk.img,format=raw,if=ide,index=2
clear:
	rm -f build/*.o build/*.elf build/codeshell.iso build/fs.img
diskreset:
	rm -f build/disk.img
	dd if=/dev/zero of=build/disk.img bs=512 count=204800
	mkfs.fat -F 12 build/disk.img
	mcopy -i build/disk.img fs/motd.txt   ::MOTD.TXT
	mcopy -i build/disk.img build/hello.elf ::HELLO.ELF
