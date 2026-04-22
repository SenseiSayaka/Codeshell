#include "shell.h"
#include "vga.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "fat12.h"
#include "pmm.h"
static const char* help_text =
	"Commands:\n"
	"  info			- show this message\n"
	"  clr			- clear the screen\n"
	"  echo[text]   	- print text\n"
	"  meminfo		- debug heap\n"
	"  fsinfo        - filesystem debug heap\n"
	"  ls            - print list of files\n"
	"  cat           - reading and print content of file\n";

static const char* skip_spaces(const char* s)
{
	while(*s == ' ') s++;
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
	}
	else if (k_strncmp(input, "clr", cmd_len) == 0 && cmd_len == 3)
	{
		Reset();
	}
	else if (k_strncmp(input, "fsinfo", cmd_len) == 0 && cmd_len == 6) {
	    fat12_dump_bpb();
	}else if(k_strncmp(input, "pageinfo", cmd_len)==0&&cmd_len==8){
		printf("\npages used: %d (%d KB)\n",
				pmm_used_pages(), pmm_used_pages()*4);
		printf("pages free: %d (%d KB)\n",
				pmm_free_pages(), pmm_free_pages()*4);
	}
	else if (k_strncmp(input, "ls", cmd_len) == 0 && cmd_len == 2) {
	    Fat12File* files = (Fat12File*) k_malloc(sizeof(Fat12File) * FAT12_MAX_FILES);
	    if (!files) { print("out of memory\n"); }
	    else {
	        int count = fat12_ls(files, FAT12_MAX_FILES);
	            }
	            k_free(files);
	        }
	      
	else if (k_strncmp(input, "cat", 3) == 0 && cmd_len >= 3)
		{		
		    const char* filename = skip_spaces(input + 3);		   		    
		    uint8_t* buf = (uint8_t*) k_malloc(4096);		    		    
		    if (!buf) { print("out of memory\n"); }
		    else {
		        
		        int bytes = fat12_read(filename, buf, 4095);		        		        
		        if (bytes < 0) {
		            print("file not found\n");
		        } else {
		            buf[bytes] = '\0';
		            print("\n");
		            print((char*)buf);
		            print("\n");
		        }
		        k_free(buf);
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
