#pragma once
#include "stdint.h"

#define FAT12_MAX_FILES 224
#define FAT12_NAME_LEN 8 
#define FAT12_EXT_LEN 3
#define ATTR_DIRECTORY 0x10
#define ATTR_VOLUME_ID 0x08

// BPB - beginning of FAT12 image
typedef struct
{
	uint8_t jump[3];
	uint8_t oem[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t fat_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t sector_per_fat;
	uint16_t sectors_per_track;
	uint16_t head_count;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;
	uint8_t drive_number;
	uint8_t reserved;
	uint8_t boot_sig;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fs_type[8];
} __attribute__((packed)) Fat12BPB;

// write into root directory
typedef struct
{
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attributes;
	uint8_t reserved[10];
	uint16_t time;
	uint16_t date;
	uint16_t first_cluster;
	uint32_t file_size;
} __attribute__((packed)) Fat12DirEntry;

typedef struct
{
	char name[13]; // FILENAME.EXT
	uint32_t size;
	uint16_t first_cluster;
	uint8_t is_dir;
} Fat12File;

void fat12_init(void* image);
int fat12_ls(Fat12File* out, uint32_t max_count);
int fat12_read(const char* filename, uint8_t* buf, uint32_t buf_size);
void fat12_dump_bpb();
