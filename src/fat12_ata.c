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
static int read_sector(uint32_t lba,uint8_t* buf){
	return ata_read(lba,1,buf);
}
static int write_sector(uint32_t lba,uint8_t* buf){
	return ata_write(lba,1,buf);
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
//write a value to FAT12
static int fat12_set_cluster(uint16_t cluster,uint16_t value){
	uint32_t offset=cluster+cluster/2;
	uint32_t fat_sector=fat_start_lba+offset/512;
	uint32_t fat_offset=offset%512;
	uint8_t buf[512];
	if(read_sector(fat_sector,buf)!=0)return -1;
	if(fat_offset==511){
		//value crosses broad of sectors
		uint8_t buf2[512];
		if(read_sector(fat_sector+1,buf2)!=0)return -1;
		if(cluster&1){
			buf[511]=(buf[511]&0x0F)|((value&0x0F)<<4);
			buf2[0]=(value>>4)&0xFF;
		}else{
			buf[511]=value&0xFF;
			buf2[0]=(buf2[0]&0xF0)|((value>>8)&0x0F);
		}
		if(write_sector(fat_sector,buf)!=0)return -1;
		if(write_sector(fat_sector+1,buf2)!=0)return -1;
	}else{
		uint16_t* slot=(uint16_t*)(buf+fat_offset);
		if(cluster&1){
			*slot=(*slot&0x000F)|(value<<4);
		}else{
			*slot=(*slot&0xF000)|(value&0x0FFF);
		}
		if(write_sector(fat_sector,buf)!=0)return -1;
		if(write_sector(fat_sector+bpb.sectors_per_fat,buf)!=0)return -1;
	}
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
//find free cluster
static uint16_t fat12_find_free_cluster(){
	uint32_t total_clusters=bpb.total_sectors_16?(bpb.total_sectors_16-data_start_lba)/bpb.sectors_per_cluster
		:(bpb.total_sectors_32-data_start_lba)/bpb.sectors_per_cluster;
	for(uint16_t c=2;c<total_clusters&&c<0xFF0;c++){
		if(fat12_ata_next_cluster(c)==0)return c;
	}
	return 0;
}
//transform "FILENAME.TXT" to format 8.3(name[8] & ext[3])
static void make_83_name(const char* filename,uint8_t* name, uint8_t* ext){
	for(int i=0;i<8;i++)name[i]=' ';
	for(int i=0;i<3;i++)ext[i]=' ';
	int i=0,j=0;
	//name
	while(filename[i]&&filename[i]!='.'&&j<8){
		char c=filename[i];
		if(c>='a'&&c<='z')c-=32;//uppercase
		name[j++]=c;
		i++;
	}
	//skip dot
	while(filename[i]&&filename[i]!='.')i++;
	if(filename[i]=='.')i++;
	//ext
	j=0;
	while(filename[i]&&j<3){
		char c=filename[i];
		if(c>='a'&&c<='z')c-=32;
		ext[j++]=c;
		i++;
	}
}
int fat12_ata_create(const char* filename,const uint8_t* data, uint32_t size){
	if(!ata_fat_ready) return-1;
	//prepare name
	uint8_t name83[8],ext83[3];
	make_83_name(filename,name83,ext83);
	//find free entry in directory
	uint8_t sector[512];
	int found_sector=-1;
	int found_index=-1;
	for(uint32_t s=0;s<root_sectors;s++){
		if(read_sector(root_start_lba+s,sector)!=0)return -1;
		Fat12AtaDirEntry* entries=(Fat12AtaDirEntry*)sector;
		for(int i=0;i<16;i++){
			//0x00=unsused, 0xE5=deleted
			if(entries[i].name[0]==0x00||entries[i].name[0]==0xE5){
				found_sector=s;
				found_index=i;
				break;
			}
		}
		if(found_sector>=0)break;
	}
	if(found_sector<0){
		print("fat12_ata: root directory full\n");
		return -1;
	}
	//allocate clusters and write data
	uint32_t cluster_size=bpb.sectors_per_cluster*512;
	uint32_t clusters_needed=(size+cluster_size-1)/cluster_size;
	if(clusters_needed==0)clusters_needed=1;
	uint16_t first_cluster=0;
	uint16_t prev_cluster=0;
	uint32_t written=0;
	for(uint32_t cn=0;cn<clusters_needed;cn++){
		uint16_t cluster=fat12_find_free_cluster();
		if(!cluster){
			print("fat12_ata: no free clusters\n");
			return -1;
		}
		//temporary mark as EOF
		if(fat12_set_cluster(cluster,0xFFF)!=0)return -1;
		if(cn==0)first_cluster=cluster;
		else fat12_set_cluster(prev_cluster,cluster);
		//write data of cluster
		uint32_t lba=cluster_to_lba(cluster);
		uint8_t buf[512];
		for(uint8_t sc=0;sc<bpb.sectors_per_cluster;sc++){
			// turn buf to null
			for(int b=0;b<512;b++)buf[b]=0;
			uint32_t copy=512;
			if(written+copy>size)copy=size-written;
			for(uint32_t b=0;b<copy;b++)buf[b]=data[written+b];
			written+=copy;
			if(write_sector(lba+sc,buf)!=0)return -1;
			if(written>=size)break;
		}prev_cluster=cluster;
	}if(read_sector(root_start_lba+found_sector,sector)!=0)return-1;
	Fat12AtaDirEntry* entries=(Fat12AtaDirEntry*)sector;
	Fat12AtaDirEntry* e=&entries[found_index];
	for(int i=0;i<8;i++)e->name[i]=name83[i];
	for(int i=0;i<3;i++)e->ext[i]=ext83[i];
	e->attributes=0x20;//ARCHIVE
	e->first_cluster=first_cluster;
	e->file_size=size;
	e->time=0;e->date=0;
	for(int i=0;i<10;i++)e->reserved[i]=0;
	if(write_sector(root_start_lba+found_sector,sector)!=0)return -1;
	return 0;
}
int fat12_ata_delete(const char* filename){
	if(!ata_fat_ready)return -1;
	uint8_t sector[512];
	for(uint32_t s=0;s<root_sectors;s++){
		if(read_sector(root_start_lba+s,sector)!=0)return -1;
		Fat12AtaDirEntry* entries=(Fat12AtaDirEntry*)sector;
		for(int i=0;i<16;i++){
			Fat12AtaDirEntry* e=&entries[i];
			if(e->name[0]==0x00)return -1; //end of directory
			if(e->name[0]==0xE5)continue;
			if(e->attributes&ATTR_VOLUME_ID)continue;
			char name[13];
			format_name(e->name,e->ext,name);
			if(k_strcmp(name,filename)!=0)continue;
			//free chain of clusters
			uint16_t cluster=e->first_cluster;
			uint32_t iter=0;
			while(cluster>=2&&cluster<0xFF8){
				if(iter++>4096){
					print("delete: chain loop\n");
					break;
				}uint16_t next=fat12_ata_next_cluster(cluster);
				fat12_set_cluster(cluster,0x000);
				cluster=next;
			}//mark entry as deleted
			 e->name[0]=0xE5;
			 if(write_sector(root_start_lba+s,sector)!=0)return -1;
			 return 0;
		}
	}return -1;
}
