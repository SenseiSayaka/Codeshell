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
	"  ps            - show current tasks state\n"
	"  rn            - rename file\n"
	"  hexdump       - hex dump of file\n"
	"  date          - show current RTC date/time\n"
	"  reboot        - reboot the system\n"
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
static void print_dd(uint8_t v){
	char buf[3];
	buf[0]='0'+(v/10);
	buf[1]='0'+(v%10);
	buf[2]='\0';
	print(buf);
}
static uint8_t rtc_read(uint8_t reg){
	outPortB(0x70,reg);
	return inPortB(0x71);
}
static uint8_t bcd2bin(uint8_t v){
	return (v>>4)*10+(v&0x0F);
}
static void date(){
	//wait, until RTC not busy(bit 7 of register A)
	uint32_t timeout=100000;
	while((rtc_read(0x0A)&0x80)&&--timeout);
	uint8_t sec=bcd2bin(rtc_read(0x00));
	uint8_t min=bcd2bin(rtc_read(0x02));
	uint8_t hour=bcd2bin(rtc_read(0x04));
	uint8_t day=bcd2bin(rtc_read(0x07));
	uint8_t mon=bcd2bin(rtc_read(0x08));
	uint8_t year=bcd2bin(rtc_read(0x09));
	hour+=3;
	if(hour>=24)hour-=24;
	print("\n");
	print_dd(day);
	print(".");
	print_dd(mon);
	print(".20");
	print_dd(year);
	print("  ");
	print_dd(hour);
	print(":");
	print_dd(min);
	print(":");
	print_dd(sec);
	print(" MSK\n");
}
static void hexdump(const char* filename){
	uint8_t* buf=(uint8_t*)k_malloc(4096);
	if(!buf){print("\nout of memory\n");k_free(buf);return;}
	int bytes=fat12_ata_read(filename,buf,4095);
	if(bytes<0){print("\nfile not found\n");k_free(buf);return;}
	print("\n");
	const char hex[]="0123456789ABCDEF";
	for(int i=0;i<bytes;i+=16){
		//addres
		char addr[6];
		addr[0]=hex[(i>>12)&0xF];
		addr[1]=hex[(i>>8)&0xF];
		addr[2]=hex[(i>>4)&0xF];
		addr[3]=hex[1&0xF];
		addr[4]=':';
		addr[5]=' ';
		for(int k=0;k<6;k++){char t[2]={addr[k],0};print(t);}
		//hex part
		for(int j=0;j<16;j++){
			if(i+j<bytes){
				uint8_t b=buf[i+j];
				char hb[3]={hex[b>>4],hex[b&0xF],' '};
				char t[2];
				t[1]=0;
				t[0]=hb[0];print(t);
				t[0]=hb[1];print(t);
				t[0]=hb[2];print(t);
			}else{
				print("   ");
			}if(j==7)print(" ");
		}print(" |");
		//ascii part
		for(int j=0;j<16&&i+j<bytes;j++){
			uint8_t b=buf[i+j];
			char t[2]={(b>=0x20&&b<0x7F)?(char)b:'.',0};
			print(t);
	}print("|\n");
    }k_free(buf);
}
static void echo(const char* arg){
	print("\n");
	if(*arg=='\0'){print("\n");return;}
	while(*arg){
		if(*arg=='\\'&&*(arg+1)){
			arg++;
			switch(*arg){
				case 'n':print("\n");break;
				case 't':print("\t");break;
				case '\\':print("\\");break;
				default:{
						char t[3]={'\\',*arg,0};print(t);break;
					}
			}
		}else{
			char t[2]={*arg,0};
			print(t);
		}arg++;
	}print("\n");
}
static void cat(const char* filename){
	uint8_t* buf=(uint8_t*)k_malloc(8192);
	if(!buf){print("\nout of memory\n");return;}
	int bytes=fat12_ata_read(filename, buf, 8191);
	if(bytes<0){print("\nfile not found\n");k_free(buf);return;}
	buf[bytes]='\0';
	int total_lines=0;
	for(int i=0;i<bytes;i++)if(buf[i]=='\n')total_lines++;
	if(total_lines<=(height-2)){
		print("\n");
		print((char*)buf);
		print("\n");
	}else{
		int line_no=0;
		int i=0;
		print("\n");
		while(i<=bytes){
			if(buf[i]=='\n'||buf[i]=='\0'){
				line_no++;
				if(line_no%(height-2)==0&buf[i]!='\0'){
					print_color("\n-- more -- (space=next page, q=quit) --",
							COLOR8_LIGHT_RED,COLOR8_BLACK);
					char ch=0;
					while(!ch)ch=inPortB(0x60);
					print("\r");
					for(int k=0;k<40;k++)print(" ");
					print("\r");
					if(ch=='q'||ch==0x10)break;
				}
			}if(buf[i]=='\0')break;
			char t[2]={(char)buf[i],0};
			print(t);
			i++;
		}print("\n");
	}k_free(buf);
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
	}else if(k_strncmp(input,"date",cmd_len)==0&cmd_len==4){
		date();
	}else if(k_strncmp(input,"reboot",cmd_len)==0&&cmd_len==6){
		print("\nrebooting...\n");
		outPortB(0x64,0xFE);
		for(;;)asm volatile("int $0x00");
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
	}else if(k_strncmp(input,"rn",cmd_len)==0&&cmd_len==2){
		const char* rest=skip_spaces(input+2);
		if(*rest=='\0'){print("\nusage: rename OLD NEW\n");}
		else{
			char old_name[16]={0};
			char new_name[16]={0};
			int i=0;
			while(*rest&&*rest!=' '&&i<15){old_name[i++]=*rest++;}
			rest=skip_spaces(rest);
			i=0;
			while(*rest&&*rest!=' '&&i<15){new_name[i++]=*rest++;}
			if(!new_name[0]){print("\nusage: rename OLD NEW\n");}
			else{
				//read old file
				uint8_t* buf=(uint8_t*)k_malloc(64*1024);
				if(!buf){print("\nout of memory\n");}
				else{
					int bytes=fat12_ata_read(old_name,buf,64*1024);
					if(bytes<0){
						print("\nfile not found\n");
					}else{
						//create with new name,delete old
						if(fat12_ata_create(new_name,buf,bytes)==0){
							fat12_ata_delete(old_name);
							printf("\nrenamed %s -> %s\n",old_name,new_name);
						}else{
							print("\nrename failed\n");
						}
					}k_free(buf);
				}
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
	    }else{
		    cat(filename);
	    }
	}	
	else if(k_strncmp(input, "echo", cmd_len) == 0 && cmd_len == 4)
	{
		echo(skip_spaces(input+4));
	}else if(k_strncmp(input,"hexdump",cmd_len)==0&&cmd_len==7){
		const char* filename=skip_spaces(input+7);
		if(*filename=='\0')print("\nusage: hexdump FILENAME\n");
		else hexdump(filename);
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
