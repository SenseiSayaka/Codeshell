# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# GDT Table
.section .data
.align 4
gdt_start:
    .long 0x00000000  # Null descriptor
    .long 0x00000000

gdt_code:
    .word 0xFFFF      # Limit (0-15)
    .word 0x0000      # Base (0-15)
    .byte 0x00        # Base (16-23)
    .byte 0x9A        # Access Byte (Present, DPL=00, S=1, Executable, Readable)
    .byte 0xCF        # Flags (Granularity=1, 32-bit) + Limit (16-19)
    .byte 0x00        # Base (24-31)

gdt_data:
    .word 0xFFFF      # Limit (0-15)
    .word 0x0000      # Base (0-15)
    .byte 0x00        # Base (16-23)
    .byte 0x92        # Access Byte (Present, DPL=00, S=1, Writable)
    .byte 0xCF        # Flags (Granularity=1, 32-bit) + Limit (16-19)
    .byte 0x00        # Base (24-31)

gdt_end:

# GDT Descriptor
gdt_descriptor:
    .word gdt_end - gdt_start - 1  # Limit (размер GDT - 1)
    .long gdt_start                # Base (адрес GDT)

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
    # Set up the stack pointer.
    movl $stack_top, %esp

    # Load the GDT.
    lgdt (gdt_descriptor)

    # Reload segment registers.
    jmp $0x08, $reload_cs  # Far jump to reload CS with the new code segment
reload_cs:
    mov $0x10, %ax          # Load data segment selector
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Call global constructors (if any).
    call _init

    # Transfer control to the main kernel.
    call kernel_main

    # Hang if kernel_main unexpectedly returns.
    cli
1:  hlt
    jmp 1b

.size _start, . - _start