#include "stdint.h"
#include "keyboard.h"

void update_cursor(uint16_t row, uint16_t column);

uint16_t row = 20;
uint16_t column = 20;

void update_cursor(uint16_t row, uint16_t column)
{
    unsigned short position = (row*80) + column;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position&0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position>>8)&0xFF));
}   

