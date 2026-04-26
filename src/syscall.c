#include "syscall.h"
#include "idt.h"
#include "util.h"
#include "vga.h"
#include "stdlib/stdio.h"
#include "task.h"
#include "keyboard.h"
// sys_write - print string from user space
// ebx - pointer of string, ecx - length
static void sys_write(struct InterruptRegisters* regs){
	const char* str=(const char*)regs->ebx;
	uint32_t len=regs->ecx;
	//basic defend - do not read from kernel space
	if((uint32_t)str < 0x1000 || (uint32_t)str >= 0xC0000000){
		regs->eax=-1;
		return;
	}
	//print char-by-char using print()
	char buf[2]={0,0};
	for(uint32_t i=0;i<len&&i<4096;i++){
		buf[0]=str[i];
		if(buf[0]=='\0')break;
		print(buf);
	}
	regs->eax=len;
}
// sys_exit - finish current task
// ebx - code of return
static void sys_exit(struct InterruptRegisters* regs){
	uint32_t code=regs->ebx;
	printf("\n[task existed with code%d]\n", code);
	print("csh>");
	setLineStart();
	task_exit();
}
static void sys_getpid(struct InterruptRegisters* regs){
	extern int get_current_task_id();
	regs->eax=get_current_task_id();
}
// main handler
void syscall_handler(struct InterruptRegisters* regs){
	switch(regs->eax){
		case SYS_EXIT:sys_exit(regs);break;
		case SYS_WRITE:sys_write(regs);break;
		case SYS_GETPID:sys_getpid(regs);break;
		default:
			printf("[syscall] unknown: %d\n", regs->eax);
			regs->eax=-1;
			break;
	}
}
void syscall_init(){extern void irq_install_handler(int irq, void(*handler)(struct InterruptRegisters*));}
