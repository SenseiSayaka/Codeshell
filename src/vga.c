#include "vga.h"

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const ) 0xB8000;
const uint16_t default_c = (COLOR8_WHITE << 8) |
(COLOR8_BLACK << 12);

uint16_t curr = default_c;

void set_screen_color(uint8_t color)
{
    uint8_t* video_memory = (uint8_t*) 0xB8000;
    for (int i = 0; i < width * height * 2; i += 2)
    {
        video_memory[i + 1] = color;
    }
}


void Reset() 
{
    line = 0;
    column = 0;
    curr = default_c;

    for(uint16_t y = 0; y < height; y++)
    {
        for(uint16_t x = 0; x < width; x++)
        {
            vga[y * width + x] = ' ' | default_c;
        }
    }
}


void newLine()
{
    if(line < height - 1)
    {
        line++;
        column = 0;
    }
    else
    {
        scrollUp();
        column = 0;
    }
}


void scrollUp()
{
    for(uint16_t y = 0; y < height; y++)
    {
        for (uint16_t x = 0; x < width; x++)
        {
            vga[(height - 1) * width + x] = ' ' | curr;
        }
    }
    for(uint16_t x = 0; x < width; x++)
    {
        vga[(height - 1) * width + x] = ' ' | curr;
    }
}


void print(const char *s)
{
    while(*s)
    {
        switch(*s)
        {
            case '\n':
                newLine();
                break;
            case '\r':
                column = 0;
                break;
            case '\t':
                if(column == width)
                {
                    newLine();
                }
                uint16_t tab = 4 - (column % 4);
                while(tab != 0)
                {
                    vga[line * width + (column++)] = ' ' | curr;
                    tab--;
                }
                break;
            default:
                if(column == width)
                {
                    newLine();
                }
                vga[line * width + (column++)] = *s | curr;
                break;
        }
        s++;
    }
}












