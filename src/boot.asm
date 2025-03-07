org 0x7C00
bits 16

%define ENDL 0x0D, 0X0A

.start:
    jmp main

puts:
    push si
    push ax
    push bx

.loop:
    lodsb
    or al, al
    jz .done

    mov ah, 0x0e
    mov bh, 0
    int 0x10
    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret
main:
    mov ax, 0
    mov ds, ax
    mov es, ax

    mov ss, ax
    mov sp, 0x7C00

    mov si, msg
    call puts

    jmp .halt
    

.halt:
    jmp .halt

msg: db "hello world", ENDL, 0

times 510-($-$$) db 8
dw 0AA55h 