bits 32

global idt_flush

idt_flush:
	mov eax, [esp+4]
	lidt [eax]
	sti
	ret



%macro ISR_NOERRCODE 1
	globalisr&1
	isr%1:
		cli
		push long 0 
		push long %1
		jmp isr_common_stub

%endmacro


%macro ISR_ERRORCODE 1
	global isr%1
	isr%1:
		cli
		push long %1
		jmp isr_common_stub

%endmacro


%macro IRQ 2
    global irq%1
    irq%1:
        CLI
        PUSH LONG 0
        PUSH LONG %2
        JMP irq_common_stub
%endmacro

ISR_ERRORCODE 0 
ISR_ERRORCODE 1 
ISR_ERRORCODE 2 
ISR_ERRORCODE 3 
ISR_ERRORCODE 4 
ISR_ERRORCODE 5 
ISR_ERRORCODE 6 
ISR_ERRORCODE 7 
ISR_ERRORCODE 8 
ISR_ERRORCODE 9 
ISR_ERRORCODE 10 
ISR_ERRORCODE 11 
ISR_ERRORCODE 12 
ISR_ERRORCODE 13 
ISR_ERRORCODE 14 
ISR_ERRORCODE 15 
ISR_ERRORCODE 16 
ISR_ERRORCODE 17 
ISR_ERRORCODE 18 
ISR_ERRORCODE 19 
ISR_ERRORCODE 20 
ISR_ERRORCODE 21 
ISR_ERRORCODE 22 
ISR_ERRORCODE 23 
ISR_ERRORCODE 24 
ISR_ERRORCODE 25 
ISR_ERRORCODE 26 
ISR_ERRORCODE 27 
ISR_ERRORCODE 28 
ISR_ERRORCODE 29 
ISR_ERRORCODE 30 
ISR_ERRORCODE 31 
ISR_ERRORCODE 128 
ISR_ERRORCODE 177

IRQ 0, 32
IRQ   1,    33
IRQ   2,    34
IRQ   3,    35
IRQ   4,    36
IRQ   5,    37
IRQ   6,    38
IRQ   7,    39
IRQ   8,    40
IRQ   9,    41
IRQ  10,    42
IRQ  11,    43
IRQ  12,    44
IRQ  13,    45
IRQ  14,    46
IRQ  15,    47

extern isr_handler

isr_common_stub:
	pusha
	mov eax, ds
	push eax
	mov eax, ds
	mov eax, cr2
	push eax
	
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	push esp
	call isr_handler

	mov esp, 8
	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa
	add esp, 8
	sti
	ret
 
extern irq_handler
irq_common_stub:
    pusha
    mov eax,ds
    PUSH eax
    MOV eax, cr2
    PUSH eax

    MOV ax, 0x10
    MOV ds, ax
    MOV es, ax
    MOV fs, ax
    MOV gs, ax

    PUSH esp
    CALL irq_handler

    ADD esp, 8
    POP ebx
    MOV ds, bx
    MOV es, bx
    MOV fs, bx
    MOV gs, bx

    POPA
    ADD esp, 8
    STI
    IRET
