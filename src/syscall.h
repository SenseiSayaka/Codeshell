#pragma once
#include "stdint.h"
#include "util.h"
//numbers of syscalls
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_GETPID 3
#define SYS_READ 4
#define SYS_GETTIME 5
void syscall_init();
void syscall_handler(struct InterruptRegisters* regs);
