BITS 32

section .text
    ALIGN 4
    DD 0x1BADB002
    DD 0x00000003
    DD -(0x1BADB002 + 0x00000003)

global start
extern kmain

start:
    CLI
    MOV esp, stack_space
    PUSH ebx
    CALL kmain
    HLT
HaltKernel:
    CLI
    HLT
    JMP HaltKernel

section .bss
RESB 8192
stack_space:
