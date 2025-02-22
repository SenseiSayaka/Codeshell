#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>

void test_gdt(void){
	uint16_t cs, ds;

    // Получаем текущие значения сегментных регистров
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile("mov %%ds, %0" : "=r"(ds));

    // Если CS == 0x08 и DS == 0x10, значит GDT загружена корректно
    if (cs == 0x08 && ds == 0x10) {
        printf("GDT loaded successfully!\n");
		printf(" ")
        printf("Segment registers updated.\n");
		printf(" ")
    } else {
        printf("GDT failed to load!\n");
		printf(" ")
    }
}
void kernel_main(void) {
	terminal_initialize();
	test_gdt();
	printf("Welcome to kernel of Codeshell\n");
	printf(" ")

}
