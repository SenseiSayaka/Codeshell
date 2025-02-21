#include <stdio.h>

#include <kernel/tty.h>

void kernel_main(void) {
	terminal_initialize();
	printf("Welcome to kernel of Codeshell\n");

	float numb = 71.8540372658965367;
	printf("number = %f ", numb);
}
