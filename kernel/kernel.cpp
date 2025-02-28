#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../terminal/tty.h"

#include "../keyboard/keyboardActivePolling.h"
#include "../keyboard/keyboardScancodeToAscii.h"

// === Main Kernel Code ===

extern "C" {
  void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Hello, kernel World!\n");

    keyboardActivePolling_init();
    uint8_t shift_pressed = 0;

    
    while(true) {
      keyboardActivePolling_loop();
      uint8_t scancode = keyboardActivePolling_lastScanCode;
      
      // check pressed/realised Shift
      if (scancode == 42 || scancode == 54) {
        shift_pressed = 1;
      } else if (scancode == (42 | 0x80) || scancode == (54 | 0x80)) {
        // When the key is released, the most significant bit is set to 1.
        shift_pressed = 0;
      } else {
        char c;
        if (shift_pressed) {
          c = kbd_US_shift[scancode];
        } else {
          c = kbd_US[scancode];
        }
        
        if (c != 0) { // only printable characters
          terminal_putchar(c);
        }
      }
    }
  }
}