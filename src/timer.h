#pragma once
#include "util.h"
void initTimer();
void onIrq0(struct InterruptRegisters *regs);
uint32_t get_timer_ticks();
