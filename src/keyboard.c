#include "stdint.h"
#include "util.h"
#include "idt.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "vga.h"

bool capsOn;
bool capsLock;
static bool extendedKey = false;

// --- буфер текущей строки ввода ---
#define INPUT_BUF_SIZE 256
static char     inputBuf[INPUT_BUF_SIZE];
static uint16_t inputLen = 0;   // сколько символов в буфере
static uint16_t inputPos = 0;   // позиция курсора внутри буфера

// позиция на экране сразу после "csh>"
static uint16_t startCol  = 0;
static uint16_t startLine = 0;

// --- вспомогательные функции ---

// перевести индекс в буфере → экранные col/line
static void bufToScreen(uint16_t pos, uint16_t *col, uint16_t *ln) {
    uint16_t abs = startCol + pos;
    *ln  = startLine + abs / width;
    *col = abs % width;
}

// перерисовать буфер начиная с позиции from и стереть символ после конца
static void redrawFrom(uint16_t from) {
    uint16_t col, ln;
    for (uint16_t i = from; i < inputLen; i++) {
        bufToScreen(i, &col, &ln);
        putCharAt(inputBuf[i], col, ln);
    }
    // стереть символ, который был в конце (после удаления)
    bufToScreen(inputLen, &col, &ln);
    putCharAt(' ', col, ln);
}

// --- scancodes / keymaps (без изменений) ---

const uint32_t UNKNOWN = 0xFFFFFFFF;
const uint32_t ESC     = 0xFFFFFFFF - 1;
const uint32_t CTRL    = 0xFFFFFFFF - 2;
const uint32_t LSHFT   = 0xFFFFFFFF - 3;
const uint32_t RSHFT   = 0xFFFFFFFF - 4;
const uint32_t ALT     = 0xFFFFFFFF - 5;
const uint32_t F1      = 0xFFFFFFFF - 6;
const uint32_t F2      = 0xFFFFFFFF - 7;
const uint32_t F3      = 0xFFFFFFFF - 8;
const uint32_t F4      = 0xFFFFFFFF - 9;
const uint32_t F5      = 0xFFFFFFFF - 10;
const uint32_t F6      = 0xFFFFFFFF - 11;
const uint32_t F7      = 0xFFFFFFFF - 12;
const uint32_t F8      = 0xFFFFFFFF - 13;
const uint32_t F9      = 0xFFFFFFFF - 14;
const uint32_t F10     = 0xFFFFFFFF - 15;
const uint32_t F11     = 0xFFFFFFFF - 16;
const uint32_t F12     = 0xFFFFFFFF - 17;
const uint32_t SCRLCK  = 0xFFFFFFFF - 18;
const uint32_t HOME    = 0xFFFFFFFF - 19;
const uint32_t UP      = 0xFFFFFFFF - 20;
const uint32_t LEFT    = 0xFFFFFFFF - 21;
const uint32_t RIGHT   = 0xFFFFFFFF - 22;
const uint32_t DOWN    = 0xFFFFFFFF - 23;
const uint32_t PGUP    = 0xFFFFFFFF - 24;
const uint32_t PGDOWN  = 0xFFFFFFFF - 25;
const uint32_t END     = 0xFFFFFFFF - 26;
const uint32_t INS     = 0xFFFFFFFF - 27;
const uint32_t DEL     = 0xFFFFFFFF - 28;
const uint32_t CAPS    = 0xFFFFFFFF - 29;
const uint32_t NONE    = 0xFFFFFFFF - 30;
const uint32_t ALTGR   = 0xFFFFFFFF - 31;
const uint32_t NUMLCK  = 0xFFFFFFFF - 32;

const uint32_t lowercase[128] = {
    UNKNOWN,ESC,'1','2','3','4','5','6','7','8',
    '9','0','-','=','\b','\t','q','w','e','r',
    't','y','u','i','o','p','[',']','\n',CTRL,
    'a','s','d','f','g','h','j','k','l',';',
    '\'','`',LSHFT,'\\','z','x','c','v','b','n','m',',',
    '.','/',RSHFT,'*',ALT,' ',CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,
    NUMLCK,SCRLCK,HOME,UP,PGUP,'-',LEFT,UNKNOWN,RIGHT,'+',
    END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

const uint32_t uppercase[128] = {
    UNKNOWN,ESC,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',CTRL,
    'A','S','D','F','G','H','J','K','L',':','"','~',LSHFT,'|',
    'Z','X','C','V','B','N','M','<','>','?',RSHFT,'*',ALT,' ',
    CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,NUMLCK,SCRLCK,HOME,UP,PGUP,'-',
    LEFT,UNKNOWN,RIGHT,'+',END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

// --- обработчик ---

void keyboardHandler(struct InterruptRegisters *regs){
    uint8_t raw = inPortB(0x60);

    if (raw == 0xE0) {
        extendedKey = true;
        return;
    }

    char     press    = raw & 0x80;
    char     scanCode = raw & 0x7F;
    uint16_t col, ln;

    if (extendedKey) {
        extendedKey = false;
        if (press == 0) {
            switch (scanCode) {
                case 0x4B:  // стрелка влево
                    if (inputPos > 0) {
                        inputPos--;
                        bufToScreen(inputPos, &col, &ln);
                        setCursorPos(col, ln);
                    }
                    return;
                case 0x4D:  // стрелка вправо — не дальше конца текста
                    if (inputPos < inputLen) {
                        inputPos++;
                        bufToScreen(inputPos, &col, &ln);
                        setCursorPos(col, ln);
                    }
                    return;
            }
        }
        return;
    }

    switch (scanCode) {
        case 1:
        case 29:
        case 56:
        case 59: case 60: case 61: case 62: case 63:
        case 64: case 65: case 66: case 67: case 68:
        case 87: case 88:
            break;
        case 42:
            capsOn = (press == 0);
            break;
        case 58:
            if (press == 0) capsLock = !capsLock;
            break;
        default:
            if (press != 0) break;

            uint32_t key = (capsOn || capsLock) ? uppercase[scanCode] : lowercase[scanCode];

            if (key == '\n') {
                // сбросить буфер и напечатать новый промпт
                inputBuf[inputLen] = '\0';
                inputLen = 0;
                inputPos = 0;
                print("\ncsh>");
                setLineStart();

            } else if (key == '\b') {
                // backspace — удалить символ слева от курсора
                if (inputPos > 0) {
                    for (uint16_t i = inputPos - 1; i < inputLen - 1; i++)
                        inputBuf[i] = inputBuf[i + 1];
                    inputLen--;
                    inputPos--;
                    redrawFrom(inputPos);
                    bufToScreen(inputPos, &col, &ln);
                    setCursorPos(col, ln);
                }

            } else if (key != UNKNOWN && key <= 0x7F) {
                // вставить символ в позицию курсора со сдвигом вправо
                if (inputLen < INPUT_BUF_SIZE - 1) {
                    for (uint16_t i = inputLen; i > inputPos; i--)
                        inputBuf[i] = inputBuf[i - 1];
                    inputBuf[inputPos] = (char)key;
                    inputLen++;
                    inputPos++;
                    redrawFrom(inputPos - 1);
                    bufToScreen(inputPos, &col, &ln);
                    setCursorPos(col, ln);
                }
            }
    }
}

void setLineStart() {
    // vga.c держит column/line как extern, читаем через setCursorPos-совместимый способ
    // поэтому просто запрашиваем текущую позицию через отдельные экстерны
    extern uint16_t column;
    extern uint16_t line;
    startCol  = column;
    startLine = line;
    inputLen  = 0;
    inputPos  = 0;
}

void initKeyboard(){
    capsOn      = false;
    capsLock    = false;
    extendedKey = false;
    irq_install_handler(1, &keyboardHandler);
}
