#include "syscall.h"
int main(){
	print("\nhello from user space!\n");
	int pid=sys_getpid();
	print("Task ID: ");
	char c='0'+pid;
	sys_write(&c,1);
	print("\n");
	return 0;
}
