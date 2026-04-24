#pragma once
#include "stdint.h"
#include "util.h"
//numbers of syscalls
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_GETPID 3
void syscall_init();
void syscall_handler(struct InterruptRegisters* regs);
