#include "stdint.h"

void memset(void *dest, char val, uint32_t count);
void outPortB(uint16_t Port, uint8_t Value);
char inPortB(uint16_t Port);

uint32_t k_strlen(const char* s);
int k_srtcmp(const char* a, const char* b);
int k_strncmp(const char* a, const char* b, uint32_t n);
char* k_strchr(const char* s, char c);


struct InterruptRegisters{
    uint32_t cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, csm, eflags, useresp, ss;
};

