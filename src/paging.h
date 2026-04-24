#pragma once
#include "stdint.h"

//flags of entries page direcotry/page table
#define PAGE_PRESENT (1<<0)
#define PAGE_WRITABLE (1<<1)
#define PAGE_USER (1<<2) // available from ring 3
#define PAGE_4MB (1<<7) //4MB page(for PSE)
//virt beginning kernel space
#define KERNEL_VIRT_BASE 0xC0000000 //3GB
extern uint32_t kernel_dir_phys;
void paging_init();
uint32_t paging_create_dir();// new page directory for process
void paging_map(uint32_t dir_phys, uint32_t virt,
		uint32_t phys, uint32_t flags); //add mapping
void paging_switch(uint32_t dir_phys); // switch CR3
void paging_free_dir(uint32_t dir_phys); //free directory of process
					 				       									     
