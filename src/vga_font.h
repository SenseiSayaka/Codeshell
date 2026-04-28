#pragma once
#include "stdint.h"
void vga_load_font(const uint8_t* font);
extern const uint8_t custom_font[256*16];
