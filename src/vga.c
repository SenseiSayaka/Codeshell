#include "vga.h"
#include "stdint.h"
#include "util.h"

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const) 0xB8000;
const uint16_t defaultColor = (COLOR8_WHITE << 8) | (COLOR8_BLUE << 12);
uint16_t currentColor = defaultColor;
void putCharAt(char c, uint16_t col, uint16_t ln) {
    vga[ln * width + col] = c | currentColor;
}

void setCursorPos(uint16_t col, uint16_t ln) {
    column = col;
    line   = ln;
    updateHardwareCursor();
}
void updateHardwareCursor() {
    uint16_t pos = line * width + column;
    outPortB(0x3D4, 0x0F);
    outPortB(0x3D5, (uint8_t)(pos & 0xFF));
    outPortB(0x3D4, 0x0E);
    outPortB(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void moveCursorLeft() {
    if (column > 0) {
        column--;
    } else if (line > 0) {
        line--;
        column = width - 1;
    }
    updateHardwareCursor();
}

void moveCursorRight() {
    if (column < width - 1) {
        column++;
    } else if (line < height - 1) {
        line++;
        column = 0;
    }
    updateHardwareCursor();
}

void Reset(){
    line = 0;
    column = 0;
    currentColor = defaultColor;

    for (uint16_t y = 0; y < height; y++){
        for (uint16_t x = 0; x < width; x++){
            vga[y * width + x] = ' ' | defaultColor;
        }
    }
    updateHardwareCursor();
}

void newLine(){
    if (line < height - 1){
        line++;
        column = 0;
    } else {
        scrollUp();
        column = 0;
    }
}

void scrollUp(){
    for (uint16_t y = 1; y < height; y++){
        for (uint16_t x = 0; x < width; x++){
            vga[(y-1) * width + x] = vga[y * width + x];
        }
    }
    for (uint16_t x = 0; x < width; x++){
        vga[(height-1) * width + x] = ' ' | currentColor;
    }
}

void print(const char* s){
    while(*s){
        switch(*s){
            case '\n':
                newLine();
                break;
            case '\r':
                column = 0;
                break;
            case '\b':
                if(column == 4) {
                    vga[line * width + (++column)] = ' ' | currentColor;
                    break;
                }
                if (column == 0 && line != 0){
                    line--;
                    column = width;
                }
                vga[line * width + (--column)] = ' ' | currentColor;
                break;
            case '\t':
                if (column == width){
                    newLine();
                }
                uint16_t tabLen = 4 - (column % 4);
                while (tabLen != 0){
                    vga[line * width + (column++)] = ' ' | currentColor;
                    tabLen--;
                }
                break;
            default:
                if (column == width){
                    newLine();
                }
                vga[line * width + (column++)] = *s | currentColor;
                break;
        }
        s++;
    }
    updateHardwareCursor();
}
