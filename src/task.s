global task_switch_asm
; void task_switch_asm(uint32_t* old_esp, uint32_t new_esp)
task_switch_asm:
	PUSH ebp
	PUSH ebx
	PUSH esi
	PUSH edi
	; stack: [edi][esi][ebx][ebp][ret][old_esp][new_esp]
	MOV eax, [esp+20]
	MOV [eax], esp
	MOV ecx, [esp+24]
	MOV esp, ecx
	POP edi
	POP esi
	POP ebx
	POP ebp
	RET
global task_usermode_enter
task_usermode_enter:
	IRET
