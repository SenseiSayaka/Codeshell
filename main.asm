org 0x7C00
bits 16

VIDEO_MEMORY equ 0xb8000
_xpos db 0
_ypos db 0

WIDTH equ 80

%define 	c_attrib 15



code_seg equ GDT_code - GDT_start




start:
	mov bp, 0x9000
	mov sp, bp 

	
    cli

    lgdt [info]

    mov eax, cr0
    or al, 1
    mov cr0, eax
	
	
    jmp code_seg:starts
    
GDT_start:                          ; must be at the end of real mode code
    GDT_null:
        dd 0x0
        dd 0x0

    GDT_code:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10011010
        db 0b11001111
        db 0x0

    GDT_data:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10010010
        db 0b11001111
        db 0x0



    end:
    info:
        dw end - GDT_start - 1
        dd GDT_start

bits 32   

starts:
	mov ebp, 0x90000
	mov esp, ebp
	call clear
	
	mov ebx, string
	
	mov edx, VIDEO_MEMORY

	 
    jmp prints
	    
prints:
	mov al, [ebx]
	mov ah, 0x0f

	cmp al, 0

	je done


	mov [edx], ax

	inc ebx
	add edx, 2

	jmp prints

done:
	call mov_curs
	jmp $ 

clear:
	pusha
	cld
	mov edi, VIDEO_MEMORY
	mov cx, 2000
	mov ah, c_attrib
	mov al, ' '
	rep stosw
	
	mov byte [_xpos], 0
	mov byte [_ypos], 0
	popa
	ret
mov_curs:
		
string: db "|-----------------     Codeshell OS    -----------------|", 0


 
times 510-($-$$) db 0
dw 0AA55h
