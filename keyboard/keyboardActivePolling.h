#include <stdint.h>

#define KBC_STATUS   0x64
#define KBC_EA       0x60

uint8_t shift_pressed = 0;
uint8_t keyboardActivePolling_lastScanCode = 0;

// -- internal funcitons

static inline unsigned char inportb (unsigned short _port) {
  unsigned char rv;
  __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
  return rv;
}

static inline void outportb (unsigned short _port, unsigned char _data) {
  __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void delay() {
  for(int i = 0; i < 1000000; i++) { // accurate time delay would require CPU ClockCycles/USec conversion
    __asm__ __volatile__ ("nop");
  }
}

static void keyboardActivePolling_sendCommand(uint8_t command) {
  while ((inportb(KBC_STATUS) & 2)) {} // wait until cmd buffer is free
  outportb(KBC_EA, command);
}

uint32_t keyboardActivePolling_getScancode() {
  static unsigned e0_code = 0;
  static unsigned e1_code = 0;
  static uint16_t e1_prev = 0;
  uint8_t scancode = 0;
  if (inportb(KBC_STATUS) & 1) {
    // a scancode is available in the buffer
    scancode = inportb(KBC_EA);
    if (e0_code == 1) {
      // scancode is an e0 code
      e0_code = 0;
      return (0xe000 | scancode);
    } else if (e1_code == 1) {
      // scancode is first byte of e1 code
      e1_prev = scancode;
      e1_code = 2;
    } else  if (e1_code == 2) {
      // scancode is second byte of e1 code (first is in e1_prev)
      e1_code = 0;
      return (0xe10000 | e1_prev << 8 | scancode);
    } else if (scancode == 0xe0) {
      e0_code = 1;
      scancode = 0;
    } else if (scancode == 0xe1) {
      e1_code = 1;
      scancode = 0;
    }
  }
  return scancode;
}

// -- interface
void keyboardActivePolling_init() {
  // empty keyboard buffer
  while (inportb(KBC_STATUS) & 1) {
    inportb(KBC_EA);
  }

  // activate keyboard
  keyboardActivePolling_sendCommand(0xF4);
  while (inportb(KBC_STATUS) & 1) {
    inportb(KBC_EA); // read (and drop) what's left in the keyboard buffer
  }

  // self-test (should answer with 0xEE)
  keyboardActivePolling_sendCommand(0xEE);
}

void keyboardActivePolling_loop() {
  uint8_t scancode = 0;
  while ((scancode = keyboardActivePolling_getScancode()) == 0)  { // loop until we get a keypress
    delay();
  }
  keyboardActivePolling_lastScanCode = scancode;
}