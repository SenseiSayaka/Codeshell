#include "stdint.h"

void memset(void *dest, char val, uint32_t count);
void portb(uint16_t port, uint8_t value);


struct regs {

	uint32_t cr2;
	uint32_t ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t int_no, error_code;
	uint32_t eip, csm_eflags, useresp, ss;
};
