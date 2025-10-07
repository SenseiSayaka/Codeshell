#include "util.h"
#include "vga.h"
#include "idt.h"
#include "timer.h"

uint64_t ticks;
const uint32_t freq = 100;

void onIrq0(struct regs* reg)
{
	ticks += 1;
	print("timer");
};

void timer()
{
	ticks = 0;
	irq_install_handler(0, &onIrq0);
	uint32_t divisor = 119318/freq;
	portb(0x43, 0x36);
	portb(0x40, (uint8_t) (divisor & 0xFF));
	portb(0x40, (uint8_t) (divisor >> 8) & 0xFF);
}
