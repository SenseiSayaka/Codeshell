#include "stdint.h"
#include "util.h"

void memset(void *dest, char val, uint32_t count){
    char *temp = (char*) dest;
    for (; count != 0; count --){
        *temp++ = val;
    }

}

void outPortB(uint16_t Port, uint8_t Value){
    asm volatile ("outb %1, %0" : : "dN" (Port), "a" (Value));
}


char inPortB(uint16_t port){
    char rv;
    asm volatile("inb %1, %0": "=a"(rv):"dN"(port));
    return rv;
}

uint32_t k_strlen(const char* s)
{
	uint32_t len = 0;
	while(*s++) len++;
	return len;
}
int k_strcmp(const char* a, const char* b)
{
	while(*a && *a == *b)
	{
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

int k_strncmp(const char* a, const char* b, uint32_t n)
{
	while(n && *a && *a == *b)
	{
		a++;
		b++;
		n--;
	}
	if(n == 0)
		return 0;
	return (unsigned char)*a - (unsigned char)*b;
}

char* k_strchr(const char* s, char c)
{
	while(*s)
	{
		if(*s == c)
			return (char*)s;
		s++;
	}
	return 0;
}
