#pragma once
#include "stdint.h"

#define COLOR8_BLACK 0
#define COLOR8_BLUE 1
#define COLOR8_GREEN 2
#define COLOR8_CYAN 3
#define COLOR8_RED 4
#define COLOR8_MAGENTA 5
#define COLOR8_BROWN 6
#define COLOR8_WHITE 7
#define COLOR8_GRAY 8
#define COLOR8_LIGHT_BLUE 9

#define width 80
#define height 25

void print(const char* s);
void scrollUp();
void newLine();
void Reset();
void set_screen_color(uint8_t color);

