bits 32

section .text
    align 4
    dd 0x01BADB002
    dd 0x000000000
    dd -(0x1BADB002 + 0x000000000)

global start

extern kernel_main

start:
    cli
    mov esp, stack_space
    call kernel_main
    hlt

hlt_kernel:
    cli
    hlt
    jmp hlt_kernel

section .bss

resb 8192

stack_space: 
     

