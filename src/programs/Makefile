CC     = gcc
NASM   = nasm
LD     = ld

CFLAGS = -m32 -fno-stack-protector -fno-builtin -nostdlib -nostdinc \
         -ffreestanding -O2

all:
	$(NASM) -f elf32 crt0.s -o build/crt0.o
	$(CC) $(CFLAGS) -c hello.c -o build/hello.o
	$(LD) -m elf_i386 -T linker.ld -o build/hello.elf \
	      build/crt0.o build/hello.o
	
	mcopy -i ../build/fs.img build/hello.elf ::HELLO.ELF

clean:
	rm -f build/*.o build/*.elf
