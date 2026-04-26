#include "ata.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "vga.h"
static AtaIdentify ata_identify_buf;
static char ata_model_buf[41];
static uint32_t total_sectors=0;
static uint8_t current_drive=0xE0;//0xA0=master, 0xB0=slave
//current chanel
static uint16_t ata_data_port=ATA_PRIMARY_DATA;
static uint16_t ata_status_port=ATA_PRIMARY_STATUS;
static uint16_t ata_cmd_port=ATA_PRIMARY_COMMAND;
static uint16_t ata_alt_port=ATA_PRIMARY_ALT_STATUS;
static uint16_t ata_sel_port=ATA_PRIMARY_DRIVE_SEL;
static uint16_t ata_lba_lo_port=ATA_PRIMARY_LBA_LO;
static uint16_t ata_lba_mid_port=ATA_PRIMARY_LBA_MID;;
static uint16_t ata_lba_hi_port=ATA_PRIMARY_LBA_HI;
static uint16_t ata_cnt_port=ATA_PRIMARY_SECTOR_CNT;
//update all ports to needed chanel
static void ata_set_channel(int secondary) {
    if (!secondary) {
        ata_data_port=ATA_PRIMARY_DATA;
        ata_status_port=ATA_PRIMARY_STATUS;
        ata_cmd_port=ATA_PRIMARY_COMMAND;
        ata_alt_port=ATA_PRIMARY_ALT_STATUS;
        ata_sel_port=ATA_PRIMARY_DRIVE_SEL;
        ata_lba_lo_port=ATA_PRIMARY_LBA_LO;
        ata_lba_mid_port=ATA_PRIMARY_LBA_MID;
        ata_lba_hi_port=ATA_PRIMARY_LBA_HI;
        ata_cnt_port=ATA_PRIMARY_SECTOR_CNT;
    } else {
        ata_data_port=ATA_SECONDARY_DATA;
        ata_status_port=ATA_SECONDARY_STATUS;
        ata_cmd_port=ATA_SECONDARY_COMMAND;
        ata_alt_port=ATA_SECONDARY_ALT_STATUS;
        ata_sel_port=ATA_SECONDARY_DRIVE_SEL;
        ata_lba_lo_port=ATA_SECONDARY_LBA_LO;
        ata_lba_mid_port=ATA_SECONDARY_LBA_MID;
        ata_lba_hi_port=ATA_SECONDARY_LBA_HI;
        ata_cnt_port=ATA_SECONDARY_SECTOR_CNT;
    }
}
//wait while BSY not over
static int ata_wait_busy(){
	uint32_t timeout=0x10000000;
	while(inPortB(ata_status_port)&ATA_SR_BSY){
		if(--timeout==0){
			print("ata: timeout waiting BSY\n");
			return -1;
		}
	}
	return 0;
}
//wait DRQ(data is ready)
static int ata_wait_drq(){
	uint32_t timeout=0x10000000;
	while(!(inPortB(ata_status_port)&ATA_SR_DRQ)){
		if(inPortB(ata_status_port)&ATA_SR_ERR){
			print("ata: error waiting DRQ\n");
			return -1;
		}
		if(--timeout==0){
			print("ata: timeout waiting DRQ\n");
			return -1;
		}
	}
	return 0;
}
// 400ns delay(reading alt status four times)
static void ata_delay(){
	inPortB(ata_alt_port);inPortB(ata_alt_port);
	inPortB(ata_alt_port);inPortB(ata_alt_port);
}
//send LBA28 command
static void ata_send_command(uint32_t lba,uint8_t count,uint8_t cmd){
	//choose master drive + elder 4 bits of LBA
	outPortB(ata_sel_port,current_drive|((lba>>24)&0x0F));
	outPortB(ata_cnt_port,count);
	outPortB(ata_lba_lo_port,(uint8_t)(lba));
	outPortB(ata_lba_mid_port,(uint8_t)(lba>>8));
	outPortB(ata_lba_hi_port,(uint8_t)(lba>>16));
	outPortB(ata_cmd_port,cmd);
}
//inportw - read 16-bit word from port
static uint16_t inPortW(uint16_t port){
	uint16_t rv;
	asm volatile("inw %1, %0": "=a"(rv) : "dN"(port));
	return rv;
}
// outportw - write 16-bit word to port
static void outPortW(uint16_t port,uint16_t val){
	asm volatile("outw %1, %0" :: "dN"(port), "a"(val));
}
int ata_init(){
	uint8_t drives[2]={0xE0, 0xF0};
	int channels[2]={0,1}; //0=primary, 1=secondary
	for(int ch=0;ch<2;ch++){
		ata_set_channel(channels[ch]);
		//reset choosing of disk
		for(int d=0;d<2;d++){
			outPortB(ata_sel_port, drives[d]);
			ata_delay();
			//send identify
			outPortB(ata_cmd_port,ATA_CMD_IDENTIFY);
			ata_delay();
			//if status=0-no disk
			if(inPortB(ata_status_port)==0){
				continue; // there's no disk
			}
			//wait BSY
			if(ata_wait_busy()!=0)continue;
			//check if it's ATA(not ATAPI)
			uint8_t mid=inPortB(ata_lba_mid_port);
			uint8_t hi=inPortB(ata_lba_hi_port);
			if(mid==0x14&&hi==0xEB){
				printf("ata: drive %d is ATAPI(CD-ROM), skipping\n",d);
				continue;
			}
			if(mid !=0||hi!=0){
				printf("ata: drive %d unknown signature 0x%x, skipping\n",d,mid,hi);
				continue;
			}
			//wait DRQ
			if(ata_wait_drq()!=0)continue;
			//read 256 word's IDENTIFY data
			uint16_t* ptr=(uint16_t*)&ata_identify_buf;
			for(int i=0;i<256;i++){
				ptr[i]=inPortW(ata_data_port);
			}
			total_sectors=ata_identify_buf.lba28_sectors;
			current_drive=drives[d];
			//print model of disk
			for(int i=0;i<40;i+=2){
				ata_model_buf[i]=ata_identify_buf.model[i+1];
				ata_model_buf[i+1]=ata_identify_buf.model[i];
			}
			ata_model_buf[40]='\0';
			printf("ata: found %s drive (sel=0x%x): %s\n",
			       drives[d]==0xE0?"master":"slave", drives[d],ata_model_buf);
			printf("ata: sectors=%d (%dMB)\n",
				total_sectors,
				total_sectors/2048);
			return 0;
		}		
	}
	print("ata: no ATA drive found\n");
	return -1;
}
static void ata_soft_reset(){
	// setup SRST bit
	outPortB(ata_alt_port,0x04);
	// wait a few
	for(volatile int i=0;i<10000;i++);
	//remove SRST
	outPortB(ata_alt_port, 0x00);
	//wait BSY
	ata_wait_busy();
	outPortB(ata_sel_port, current_drive);
	ata_delay();
}
int ata_read(uint32_t lba,uint8_t count,uint8_t* buf){
	outPortB(ata_sel_port, current_drive);
	ata_delay();
	if(ata_wait_busy()!=0)return -1;
	ata_send_command(lba,count,ATA_CMD_READ_PIO);
	ata_delay();
	for(int s=0;s<count;s++){
		if(ata_wait_busy()!=0)return -1;
		if(ata_wait_drq()!=0)return -1;
		// read 256 words(512 byte = 1 sector)
		uint16_t* dst=(uint16_t*)(buf+s*512);
		for(int i=0;i<256;i++){
			dst[i]=inPortW(ata_data_port);
		}
	}
	return 0;
}
int ata_write(uint32_t lba,uint8_t count,uint8_t* buf){
	if(ata_wait_busy()!=0)return -1;
	ata_send_command(lba,count,ATA_CMD_WRITE_PIO);
	ata_delay();
	for(int s=0;s<count;s++){
		if(ata_wait_drq()!=0)return -1;
		//write 256 words
		uint16_t* src=(uint16_t*)(buf+s*512);
		for(int i=0;i<256;i++){
			outPortW(ata_data_port,src[i]);
		}
		//flush cache
		outPortB(ata_cmd_port, 0xE7);
		if(ata_wait_busy()!=0)return -1;
	}
	return 0;
}
uint32_t ata_sector_count(){
	return total_sectors;
}
uint8_t ata_get_current_drive() { return current_drive; }
