#include "elf.h"
#include "paging.h"
#include "pmm.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "vga.h"
#include "task.h"
#define PAGE_ALIGN_DOWN(x) ((x)&~(PAGE_SIZE-1))
#define PAGE_ALIGN_UP(x) (((x)+PAGE_SIZE-1)&~(PAGE_SIZE-1))
ElfLoadResult elf_load(const uint8_t* data, uint32_t size){
	ElfLoadResult result={0,0,-1};
	//check minimal size
	if(size<sizeof(ElfHeader)){
		print("elf: file too small.\n");
		return result;
	}
	ElfHeader* hdr=(ElfHeader*)data;
	//check magic
	if(hdr->magic!=ELF_MAGIC){
		print("elf: invalid mgaic.\n");
		return result;
	}
	//check if it's 32-bit x86 executable
	if(hdr->bits!=1||hdr->machine!=3||hdr->type!=2){
		print("elf: not a 32-bit x86 executable.\n");
		return result;
	}
	uint32_t page_dir=paging_create_dir();
	for(uint16_t i=0;i<hdr->phnum;i++){
		ElfProgramHeader* ph=(ElfProgramHeader*)
			(data+hdr->phoff+i*hdr->phentsize);
		if(ph->type!=PT_LOAD)continue;
		if(ph->memsz==0)continue;
		//checkif segment in user space
		if(ph->vaddr>=0xC0000000){
			print("elf: segment in kernel space.\n");
			paging_free_dir(page_dir);
			return result;
		}
		printf("elf: loading segment vaddr=0x%x filesz=%d memsz=%d\n",
				ph->vaddr, ph->filesz, ph->memsz);
		//allocate and map pages for segment
		uint32_t virt_start=PAGE_ALIGN_DOWN(ph->vaddr);
		uint32_t virt_end=PAGE_ALIGN_UP(ph->vaddr+ph->memsz);
		for(uint32_t virt=virt_start;virt<virt_end;virt+=PAGE_SIZE){
			uint32_t phys=pmm_alloc();
			if(!phys){
				print("elf: out of memory\n");
				paging_free_dir(page_dir);
				return result;
			}
			//PAGE_USER to ring 3 could appeal
			paging_map(page_dir,virt,phys,
					PAGE_PRESENT|PAGE_WRITABLE|PAGE_USER);
		}
		if(ph->filesz>0){
			elf_copy_to_user(page_dir,ph->vaddr,
					data+ph->offset,ph->filesz);
		}
		if(ph->memsz>ph->filesz){
			elf_zero_user(page_dir,
					ph->vaddr+ph->filesz,
					ph->memsz-ph->filesz);
		}
	}
	result.entry=hdr->entry;
	result.page_dir=page_dir;
	result.error=0;
	return result;
}
void elf_copy_to_user(uint32_t dir_phys, uint32_t virt,
		const uint8_t* src, uint32_t len){
	uint32_t* pd=(uint32_t*)dir_phys;
	while(len>0){
		uint32_t pdi=virt>>22;
		uint32_t pti=(virt>>12)&0x3FF;
		uint32_t offset=virt&0xFFF;
		uint32_t* pt=(uint32_t*)(pd[pdi]&~0xFFF);
		uint8_t* page=(uint8_t*)(pt[pti]&~0xFFF);
		uint32_t chunk=PAGE_SIZE-offset;
		if(chunk>len)chunk=len;
		uint8_t* dst=page+offset;
		for(uint32_t i=0;i<chunk;i++)dst[i]=src[i];
		virt+=chunk;
		src+=chunk;
		len-=chunk;
	}
}
void elf_zero_user(uint32_t dir_phys, uint32_t virt, uint32_t len){
	uint32_t* pd=(uint32_t*)dir_phys;
	while(len>0){
		uint32_t pdi=virt>>22;
		uint32_t pti=(virt>>12)&0x3FF;
		uint32_t offset=virt&0xFFF;
		uint32_t* pt=(uint32_t*)(pd[pdi]&~0xFFF);
		uint8_t* page=(uint8_t*)(pt[pti]&~0xFFF);
		uint32_t chunk=PAGE_SIZE-offset;
		if(chunk>len)chunk=len;
		uint8_t* dst=page+offset;
		for(uint32_t i=0;i<chunk;i++)dst[i]=0;
		virt+=chunk;
		len-=chunk;
	}
}
