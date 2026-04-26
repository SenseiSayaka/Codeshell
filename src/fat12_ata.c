#include "fat12_ata.h"
#include "ata.h"
#include "kmalloc.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "vga.h"
//BPB reading from first sector
typedef struct{
	uint8_t jump[3];
	uint8_t oem[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t fat_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t head_count;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;
} __attribute__ ((packed)) Fat12AtaBPB;
typedef struct{
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attributes;
	uint8_t reserved[10];
	uint16_t time;
	uint16_t date;
	uint16_t first_cluster;
	uint32_t file_size;
} __attribute__ ((packed)) Fat12AtaDirEntry;
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
static Fat12AtaBPB bpb;
static uint32_t fat_start_lba=0;
static uint32_t root_start_lba=0;
static uint32_t data_start_lba=0;
static uint32_t root_sectors=0;
static int ata_fat_ready=0;
//read 1 sector from disk
static int read_sector(uint32_t lba, uint8_t* buf){
	return ata_read(lba,1,buf);
}
int fat12_ata_init(){
	uint8_t sector[512];
	//read BPB from sector 0
	if(read_sector(0,sector)!=0){
		print("fat12_ata: failed to read BPB\n");
		return -1;
	}
	//copy BPB
	uint8_t* src=sector;
	uint8_t* dst=(uint8_t*)&bpb;
	for(uint32_t i=0;i<sizeof(Fat12AtaBPB);i++)dst[i]=src[i];
	//diagnotic
    printf("fat12_ata: bps=%d spc=%d res=%d fats=%d spf=%d\n",
           bpb.bytes_per_sector,
           bpb.sectors_per_cluster,
           bpb.reserved_sectors,
           bpb.fat_count,
           bpb.sectors_per_fat);

    if(bpb.bytes_per_sector != 512) {
        printf("fat12_ata: bad sector size: %d\n", bpb.bytes_per_sector);
        return -1;
    }
	fat_start_lba=bpb.reserved_sectors;
	root_start_lba=fat_start_lba+bpb.fat_count*bpb.sectors_per_fat;
	root_sectors=(bpb.root_entry_count*32+511)/512;
	data_start_lba=root_start_lba+root_sectors;
	printf("fat12_ata: fat=%d root=%d data=%d\n",
			fat_start_lba,root_start_lba,data_start_lba);
	ata_fat_ready=1;
	return 0;
}
//next cluster from FAT
static uint16_t fat12_ata_next_cluster(uint16_t cluster){
	uint32_t offset=cluster+cluster/2;
	uint32_t fat_sector=fat_start_lba+offset/512;
	uint32_t fat_offset=offset%512;
	uint8_t buf[512];
	if(read_sector(fat_sector,buf)!=0)return 0xFFF;
	uint16_t val;
	if(fat_offset==511){
		//recording crosses broad of sectors
		val=buf[511];
		uint8_t buf2[512];
		if(read_sector(fat_sector+1,buf2)!=0)return 0xFFF;
		val|=((uint16_t)buf2[0]<<8);
	}else{
		val=*(uint16_t*)(buf+fat_offset);
	}
	return (cluster&1)?(val>>4):(val&0x0FFF);
}
//transfer nubmer of cluster to lba
static uint32_t cluster_to_lba(uint16_t cluster){
	return data_start_lba+(cluster-2)*bpb.sectors_per_cluster;
}
static void format_name(const uint8_t* raw_name,const uint8_t* raw_ext,char* out){
	int i=0,j=0;
	while(i<8&&raw_name[i]!=' ')out[j++]=raw_name[i++];
	if(raw_ext[0]!= ' '){
		out[j++]='.';
		int k=0;
		while(k<3&&raw_ext[k]!=' ')out[j++]=raw_ext[k++];
	}
	out[j]='\0';
}
int fat12_ata_ls(char names[][13],uint32_t* sizes,int max){
	if(!ata_fat_ready)return -1;
	uint8_t sector[512];
	int found=0;
	for(uint32_t s=0;s<root_sectors&&found<max;s++){
		if(read_sector(root_start_lba+s,sector)!=0)break;
		Fat12AtaDirEntry* entries=(Fat12AtaDirEntry*)sector;
		int per_sector=512/32;
		for(int i=0;i<per_sector&&found<max;i++){
			Fat12AtaDirEntry* e=&entries[i];
			if(e->name[0]==0x00)return found; // end of directory
			if(e->name[0]==0xE5)continue; //deleted
			if(e->attributes&ATTR_VOLUME_ID)continue;
			format_name(e->name,e->ext,names[found]);
			sizes[found]=e->file_size;
			found++;
		}
	}
	return found;
}
int fat12_ata_read(const char* filename, uint8_t* buf, uint32_t buf_size){
	if(!ata_fat_ready)return -1;
	uint8_t sector[512];
	//search file in root directory;
	for(uint32_t s=0;s<root_sectors;s++){
		if(read_sector(root_start_lba+s,sector)!=0)return -1;
		Fat12AtaDirEntry* entries=(Fat12AtaDirEntry*)sector;
		int per_sector=512/32;
		for(int i=0;i<per_sector;i++){
			Fat12AtaDirEntry* e=&entries[i];
			if(e->name[0]==0x00)return -1;
			if(e->name[0]==0xE5)continue;
			if(e->attributes&ATTR_VOLUME_ID)continue;
			char name[13];
			format_name(e->name,e->ext,name);
			if(k_strcmp(name,filename)!=0)continue;
			//file found- read clusters;
			uint32_t written=0;
			uint16_t cluster=e->first_cluster;
			uint32_t file_size=e->file_size;
			uint32_t iter=0;
			while(cluster>=2&&cluster<0xFF8&&written<buf_size&&written<file_size){
				if(iter++>4096){print("fat12_ata: chain loop\n");break;}
				uint32_t lba=cluster_to_lba(cluster);
				for(uint8_t sc=0;sc<bpb.sectors_per_cluster;sc++){
					if(written>=buf_size||written>=file_size)break;
					if(read_sector(lba+sc,sector)!=0)return -1;
					uint32_t copy=512;
					if(written+copy>buf_size)copy=buf_size-written;
					if(written+copy>file_size)copy=file_size-written;
					uint8_t* dst=buf+written;
					for(uint32_t b=0;b<copy;b++)dst[b]=sector[b];
					written+=copy;
				}
				cluster=fat12_ata_next_cluster(cluster);
			}
			return (int)written;
		}
	}
	return -1;
}


