#include "shell.h"
#include "vga.h"
#include "util.h"
#include "task.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "fat12.h"
#include "pmm.h"
#include "elf.h"
#include "ata.h"
#include "fat12_ata.h"
#include "timer.h"
static const char* help_text =
	"Commands:\n"
	"  ---------------------------------------------------------------\n"
	"  info		    - show this message\n"
	"  clr		    - clear the screen\n"
	"  echo[text]    - print text\n"
	"  meminfo	    - debug heap\n"
	"  fsinfo        - filesystem debug heap\n"
	"  pageinfo	    - informations of free or used pages\n"
	"  ls            - print list of files\n"
	"  cat           - reading and print content of file\n"
	"  run           - run ELF binary program\n"
	"  diskinfo	    - debug informations of current disks\n"
	"  exit          - logout from Codeshell\n"
	"  write         - write a data into file\n"
	"  rm            - delete file\n"
	"  uptime        - show system uptime\n"
	"  ps            = show current tasks state\n"
	"  --------------------------------------------------------------\n";

static const char* skip_spaces(const char* s)
{
	while(*s == ' ')s++;
	return s;
}
static uint32_t word_len(const char* s)
{
	uint32_t n = 0;
	while(*s && *s != ' ')
	{
		s++;
		n++;
	}
	return n;
}

void shell_exec(const char* input)
{
	input = skip_spaces(input);

	if(*input == '\0')
		return;

	uint32_t cmd_len = word_len(input);
	
	if(k_strncmp(input, "info", cmd_len) == 0 && cmd_len == 4)
	{
		print("\n");
		print(help_text);
	}else if(k_strncmp(input,"uptime",cmd_len)==0&&cmd_len==6){
		uint32_t t=get_timer_ticks();
		uint32_t seconds=t/100;
		uint32_t minutes=seconds/60;
		uint32_t hours=minutes/60;
		printf("\nuptime: %d:%d:%d (%d ticks)\n",hours,minutes%60,seconds%60,t);
	}
	else if (k_strncmp(input, "clr", cmd_len) == 0 && cmd_len == 3)
	{
		Reset();
	}else if(k_strncmp(input,"ps",cmd_len)==0&&cmd_len==2){
		print("\n  PID  STATE     NAME\n");
		print(  "  ---  --------  --------\n");
		int count=task_get_count();
		for(int i=0;i<count;i++){
			char name[32];
			uint8_t state;
			task_get_info(i,name,&state);
			const char* state_str="?";
			switch(state){
				case 0:state_str="RUNNING";break;
				case 1:state_str="READY  ";break;
				case 2:state_str="DEAD   ";break;
				case 3:state_str="BLOCKED";break;
			}printf("  %d    %s  %s\n",i,state_str,name);
		}
	}
	else if (k_strncmp(input, "fsinfo", cmd_len) == 0 && cmd_len == 6) {
	    fat12_dump_bpb();
	}else if(k_strncmp(input,"diskinfo",cmd_len)==0&&cmd_len==8){
		printf("\ndisk sectors: %d\n", ata_sector_count());
		printf("disk size: %d MB\n", ata_sector_count()/2048);
	}else if(k_strncmp(input,"exit",cmd_len)==0&&cmd_len==4){
		print("\nshutting down...\n");
		//QEMU shutdown port
		outPortB(0x604,0x00);
		outPortB(0x605,0x20);
	}else if(k_strncmp(input,"rm",cmd_len)==0&&cmd_len==2){
		const char* filename=skip_spaces(input+2);
		if(*filename=='\0'){
			print("\nusage: rm FILENAME\n");
		}else{
			if(fat12_ata_delete(filename)==0){
				printf("\nfile %s deleted\n",filename);
			}else{
				print("\nfile not found\n");
			}
		}
	}else if(k_strncmp(input,"write",5)==0&&cmd_len==5){
		//format: write FILENAME text
		const char* rest=skip_spaces(input+5);
		if(*rest=='\0'){
			print("\nusage: write FILENAME text\n");
		}else{
			char filename[16];
			int i=0;
			while(*rest&&*rest!=' '&&i<15){
				filename[i++]=*rest++;
			}
			filename[i]='\0';
			rest=skip_spaces(rest);
			uint32_t len=k_strlen(rest);
			print("\n");
			if(fat12_ata_create(filename,(const uint8_t*)rest,len)==0){
				printf("file %s created (%d bytes)\n", filename,len);
			}else{
				print("write failed\n");
			}
		}
	}
	else if(k_strncmp(input, "pageinfo", cmd_len)==0&&cmd_len==8){
		printf("\npages used: %d (%d KB)\n",
				pmm_used_pages(), pmm_used_pages()*4);
		printf("pages free: %d (%d KB)\n",
				pmm_free_pages(), pmm_free_pages()*4);
	}else if (k_strncmp(input, "run", cmd_len) == 0 && cmd_len == 3) {
	    const char* filename = skip_spaces(input + 3);
	    if (*filename == '\0') {
	        print("\nusage: run FILENAME\n");
	    } else {
	        uint8_t* buf = (uint8_t*) k_malloc(64 * 1024);
	        if (!buf) {
	            print("\nout of memory\n");
	        } else {
	            int bytes = fat12_ata_read(filename, buf, 64 * 1024);
	            if (bytes < 0) {
	                print("\nfile not found\n");
	            } else {
	                print("\n");
	                ElfLoadResult res = elf_load(buf, bytes);
	                if (res.error != 0) {
	                    print("elf load failed\n");
	                } else {
	                    printf("elf loaded, entry=0x%x\n", res.entry);
	                    task_create_user(filename, res.entry, res.page_dir);
	                }
	            }
	            k_free(buf);
	        }
	    }
	}else if (k_strncmp(input,"ls",cmd_len)==0&&cmd_len==2) {
	    char names[64][13];
	    uint32_t sizes[64];
	    int count=fat12_ata_ls(names,sizes,64);
	    if (count<0) {
	        print_color("\nfs not ready\n",COLOR8_LIGHT_RED,COLOR8_BLACK);
	    } else {
	        print("\n");
	        for (int i = 0;i<count;i++) {
	            const char* name=names[i];
	            int len=0;
	            while (name[len])len++;
	            uint8_t color=COLOR8_WHITE;
	            if (len>4&&name[len-4]=='.'&& 
	                name[len-3]=='E'&&name[len-2]=='L'&&name[len-1]=='F') {
	                color=COLOR8_LIGHT_GREEN;
	            }
	            else if (len>4&&name[len-4]=='.'&&
	                     name[len-3]=='T'&&name[len-2]=='X'&&name[len-1]=='T') {
	                color=COLOR8_LIGHT_CYAN;
	            }
	            print("  ");
	            print_color(name,color,COLOR8_BLACK);
	            char buf[32];
	            int j=0;
	            buf[j++]=' ';buf[j++]=' ';
	            buf[j++]='(';
	            uint32_t s=sizes[i];
	            char numbuf[16];int ni=0;
	            if (s == 0)numbuf[ni++]='0';
	            while (s>0) { numbuf[ni++]='0'+(s % 10);s/= 10; }
	            while (--ni >= 0)buf[j++]=numbuf[ni];
	            buf[j++] = ' ';buf[j++] = 'b'; buf[j++] = ')';
	            buf[j++] = '\n';buf[j] = '\0';
	            print_color(buf,COLOR8_DARK_GREY,COLOR8_BLACK);
	        }
	    }
	}else if (k_strncmp(input, "cat", 3) == 0 && cmd_len >= 3) {
	    const char* filename = skip_spaces(input + 3);
	    if (*filename == '\0') {
	        print("\nusage: cat FILENAME\n");
	    } else {
	        uint8_t* buf = (uint8_t*) k_malloc(4096);
	        if (!buf) {
	            print("\nout of memory\n");
	        } else {
	            int bytes = fat12_ata_read(filename, buf, 4095);
	            if (bytes < 0) {
	                print("\nfile not found\n");
	            } else {
	                buf[bytes] = '\0';
	                print("\n");
	                print((char*) buf);
	                print("\n");
	            }
	            k_free(buf);
	        }
		}
	}	
	else if(k_strncmp(input, "echo", cmd_len) == 0 && cmd_len == 4)
	{
		const char* arg = skip_spaces(input + 4);
		print("\n");
		if(*arg != '\0')
			print(arg);
		print("\n");
	}
	else if(k_strncmp(input, "meminfo", cmd_len) == 0 && cmd_len == 7)
	{
		int ok=heap_check();
		if(ok==0){
			print("\nheap integrity: OK\n");
		}else{
			print("\nheap integrity: CORRUPTED!\n");
		}
		heap_dump();
	}
	else
	{
		print("\nunknown command: ");
		print(input);
		print("\n");
	}
}
