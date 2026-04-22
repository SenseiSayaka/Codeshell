#include "pmm.h"
#include "util.h"
#include "stdlib/stdio.h"
// 1 bit = 1 page (4KB)
// 128KB bitmap covers 128*1024*8 * 4KB = 4GB
#define BITMAP_SIZE (1024 * 128)
static uint8_t bitmap[BITMAP_SIZE];
static uint32_t total_pages=0;
static uint32_t used_pages=0;
//bits
static void bit_set(uint32_t page){
	bitmap[page/8] |= (1<<(page%8));
}
static void bit_clear(uint32_t page){
	bitmap[page/8] &= ~(1<<(page%8));
}
static int bit_test(uint32_t page){
	return bitmap[page/8] & (1<<(page%8));
}
// init
// mem_upper - top memory in KB(from multiboot mbi->mem_upper)
void pmm_init(uint32_t mem_upper){
	//general capasity of memory: 1MB(lower) + mem_upper KB
	uint32_t mem_bytes=(1024+mem_upper)*1024;
	total_pages=mem_bytes / PAGE_SIZE;
	// mark all as busy
	memset(bitmap,0xFF,BITMAP_SIZE);
	//free available top memory starting from 1MB
	//(lower 1MB- reserved: BIOS,VGA,kernel)
	uint32_t start_page=(1024*1024)/PAGE_SIZE;
	for(uint32_t i=start_page; i<total_pages&&i<BITMAP_SIZE*8;i++){
		bit_clear(i);
	}
	//again mark as busy first 8MB-there live kernel and stack
	//FAT12 img and heap.8MB with reserve covers all
	uint32_t kernel_pages=(8*1024*1024)/PAGE_SIZE;
	for(uint32_t i=0;i<kernel_pages;i++){
		bit_set(i);
	}
	used_pages=kernel_pages;
	printf("pmm: total=%dMB free=%dMB\n",
			(total_pages*PAGE_SIZE)/(1024*1024),
			((total_pages-used_pages)*PAGE_SIZE)/(1024*1024));
}
//allocate 1 page
uint32_t pmm_alloc(){
	for(uint32_t i=0;i<total_pages;i++){
		if(!bit_test(i)){
			bit_set(i);
			used_pages++;
			return i*PAGE_SIZE;
		}
	}
	printf("pmm_alloc: out of memory.\n");
	return 0;
}
// free page
void pmm_free(uint32_t addr){
	uint32_t page=addr/PAGE_SIZE;
	if(bit_test(page)){
		bit_clear(page);
		used_pages--;
	}
}
uint32_t pmm_used_pages(){ return used_pages; }
uint32_t pmm_free_pages(){ return total_pages - used_pages; }
