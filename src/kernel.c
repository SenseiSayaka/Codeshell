#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "util.h"
void kmain(void);
void set_screen_color(uint8_t color);

void set_screen_color(uint8_t color) {
    uint8_t* video_memory = (uint8_t*)0xB8000;

    for (int i = 0; i < width * height * 2; i += 2) {
        video_memory[i + 1] = color; // Set the background color byte
    }
}

void kmain(void) {
  initGdt();
  print("GDT is done\r\n");
  initIdt();
  print("IDT is done\r\n");
  initTimer();
  print("Timer is done\r\n"); 
  set_screen_color(0x1F);
  for(;;);
}
