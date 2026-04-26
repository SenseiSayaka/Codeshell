#include "paging.h"
#include "pmm.h"
#include "util.h"
#include "stdlib/stdio.h"
// page directory/page tabke - array from 1024 uint32_t
#define PD_INDEX(virt) ((virt)>>22)//bits 31-22
#define PT_INDEX(virt) (((virt)>>12)&0x3FF)//bits 21-22
uint32_t kernel_dir_phys=0;
#define RECURSIVE_ENTRY 1023
#define TEMP_MAP_VIRT 0xFF800000
//read/write physical page through temp mapping
static uint32_t* phys_to_virt_temp(uint32_t phys){
	return (uint32_t*) phys;
}
void paging_init(){
	// dedicate page directory of kernel
	kernel_dir_phys=pmm_alloc();
	uint32_t* pd=phys_to_virt_temp(kernel_dir_phys);
	memset(pd,0,PAGE_SIZE);
	for (uint32_t i=0;i<16;i++) {
        pd[i]=(i*0x400000)|PAGE_PRESENT|PAGE_WRITABLE|PAGE_4MB;
    }
	// identity map of first 8MB
	// using 4MB page(PSE) to avoid page tables
	// 0x00000000 - 0x00400000 -> recording 0
	// 0x00400000 - 0x00800000 -> recording 1
	pd[0]=0x00000000|PAGE_PRESENT|PAGE_WRITABLE|PAGE_4MB;
	pd[1]=0x00400000|PAGE_PRESENT|PAGE_WRITABLE|PAGE_4MB;
	//also map the kernel to kernel space(0xC00000000)
	pd[PD_INDEX(KERNEL_VIRT_BASE)]=0x00000000|PAGE_PRESENT|PAGE_WRITABLE|PAGE_4MB;
	pd[PD_INDEX(KERNEL_VIRT_BASE)+1]=0x00400000|PAGE_PRESENT|PAGE_WRITABLE|PAGE_4MB;
	
	// turn on PSE in CR4
	asm volatile("mov %%cr4, %%eax\n"
		     "or $0x10, %%eax\n" //PSE bit
		     "mov %%eax, %%cr4\n"
		     ::: "eax"
		     );
	//load page directory
	asm volatile("mov %0, %%cr3" :: "r"(kernel_dir_phys));
	// turn on paging in cr0
	asm volatile("mov %%cr0, %%eax\n"
		     "or $0x80000000, %%eax\n"
		     "mov %%eax, %%cr0\n"
		     ::: "eax"
		     );
	printf("paging: enabled, kernel_dir=0x%x\n", kernel_dir_phys);
}
//create new page directory for process
uint32_t paging_create_dir(){
	uint32_t dir_phys=pmm_alloc();
	uint32_t* pd=(uint32_t*)dir_phys;
	memset(pd,0,PAGE_SIZE);
	//copy kernel's mappings form kernel_dir
	uint32_t* kernel_pd=(uint32_t*)kernel_dir_phys;
	pd[0] = kernel_pd[0];
    pd[1] = kernel_pd[1];
	uint32_t ki=PD_INDEX(KERNEL_VIRT_BASE);
	for(uint32_t i=ki;i<1024;i++){
		pd[i]=kernel_pd[i];
	}
	return dir_phys;
}
//add mapping virt->phys to selected page directory
void paging_map(uint32_t dir_phys, uint32_t virt, uint32_t phys, uint32_t flags){
	uint32_t* pd= (uint32_t*)dir_phys;
	uint32_t pdi=PD_INDEX(virt);
	uint32_t pti=PT_INDEX(virt);
	//if page table don't exist - create
	if(!(pd[pdi]&PAGE_PRESENT)){
		uint32_t pt_phys=pmm_alloc();
		memset((void*)pt_phys,0,PAGE_SIZE);
		pd[pdi]=pt_phys|PAGE_PRESENT|PAGE_WRITABLE|PAGE_USER;
	}
	uint32_t* pt=(uint32_t*)(pd[pdi]&~0xFFF);
	pt[pti]=(phys&~0xFFF)|flags|PAGE_PRESENT;
	asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}
void paging_switch(uint32_t dir_phys){
	asm volatile("mov %0, %%cr3" :: "r"(dir_phys) : "memory");
}
//free all page tables of process(do not touch kernel's mappings)
void paging_free_dir(uint32_t dir_phys){
	uint32_t* pd=(uint32_t*)dir_phys;
	uint32_t ki=PD_INDEX(KERNEL_VIRT_BASE);
	// free only user sapce page tables
	for(uint32_t i=0;i<ki;i++){
		if(pd[i]&PAGE_PRESENT){
			uint32_t pt_phys=pd[i]&~0xFFF;
			uint32_t* pt=(uint32_t*)pt_phys;
			//free physical pages
			for(uint32_t j=0;j<1024;j++){
				if(pt[j]&PAGE_PRESENT){
					pmm_free(pt[j]&~0xFFF);
				}
			}
			pmm_free(pt_phys);
		}
	}
	pmm_free(dir_phys);
}





