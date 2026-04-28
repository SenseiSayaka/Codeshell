#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "multiboot.h"
#include "fat12.h"
#include "task.h"
#include "pmm.h"
#include "paging.h"
#include "syscall.h"
#include "ata.h"
#include "fat12_ata.h"
#include "vga_font.h"
void kmain(MultibootInfo* mbi);
static uint8_t diag_sector[512];
void set_screen_color(uint8_t color);

void set_screen_color(uint8_t color) {
    uint8_t* video_memory = (uint8_t*)0xB8000;

    for (int i = 0; i < width * height * 2; i += 2) {
        video_memory[i + 1] = color; // Set the background color byte
    }
}
void task_counter(){
	uint32_t count=0;
	while(1){
		uint16_t* vga=(uint16_t*)0xB8000;
		uint16_t color=0x0F00;
		vga[79]=color|('0'+(count%10));
		vga[78]=color|('0'+((count/10)%10));
		for(volatile int i=0; i<100000;i++);
		count++;
	}
}
extern int kernel_end;
void kmain(MultibootInfo* mbi) {
  Reset();
  set_screen_color(0x1F);
  initGdt();
  initIdt();
  tasks_init();
  initTimer();
  initKeyboard();
  uint32_t heap_base = (uint32_t) &kernel_end;
	if (mbi->flags & MB_FLAG_MODS && mbi->mods_count > 0) {
	    MultibootModule* mod = (MultibootModule*) mbi->mods_addr;
	    printf("fs.img at 0x%x, size=%d\n",
	           mod->mod_start,
	           mod->mod_end - mod->mod_start);
	    fat12_init((void*) mod->mod_start);
	    if (mod->mod_end > heap_base)
            heap_base = (mod->mod_end + 0xFFF) & ~0xFFF;
	} else {
	    print("warning: no multiboot modules found\n");
	}
  heap_init(heap_base);
  // PMM-transfer upper memory form multiboot
  if(mbi->flags& MB_FLAG_MEM){
	  pmm_init(mbi->mem_upper);
  }else{
	  pmm_init(32*1024); //fallback: 32MB
  }
  paging_init();
  syscall_init();
  ata_init();
  fat12_ata_init();
  printf("heap base: 0x%x\n", heap_base);
  task_create("counter",task_counter);
    uint8_t* motd = (uint8_t*) k_malloc(512);
    if (motd) {
        int bytes = fat12_ata_read("MOTD.TXT", motd, 511);
        if (bytes > 0) {
            motd[bytes] = '\0';
            print((char*)motd);
        }
        k_free(motd);
    }
  print("csh>"); 
  setLineStart();
  vga_load_font(custom_font);
  set_screen_color(0x0F);
  for(;;);
}
