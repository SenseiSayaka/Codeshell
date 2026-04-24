OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text  : { *(.text)  }
    .data  : { *(.data)  }
    .bss   : { *(.bss)   }
}
