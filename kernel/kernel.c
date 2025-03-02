#include "../interrupts/common.h"
#include "../interrupts/gdt.h"
#include "../interrupts/idt.h"
#include "../interrupts/isrs.h"

#include "../terminal/tty.h"

#include "../keyboard/keyboardActivePolling.h"
#include "../keyboard/keyboardScancodeToAscii.h"

#define IRQ(x) x+32 // also in isrs.h

#if defined(__cplusplus)
extern "C"
#endif

#define IRQ(x) x+32 // also in isrs.h

void kb_interrupt(struct regs *r) {
  terminal_writestring("KEY");
}

void kernel_main() {
  gdt_install();
  idt_install();
  isrs_install();
  terminal_initialize();
  register_interrupt_handler(IRQ(1), &kb_interrupt);

  terminal_writestring("Hello, kernel World!\n");
  keyboardActivePolling_init();

  while(true) {
    uint8_t scancode = 0;
    scancode = keyboardActivePolling_getScancode(); // required otherwise the keyboard event fires only once !?

    delay();
    terminal_putchar('.');
  }

}