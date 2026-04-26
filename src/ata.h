#pragma once
#include "stdint.h"
//ports of first ATA chanel(Primary)
#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_ERROR 0x1F1
#define ATA_PRIMARY_SECTOR_CNT 0x1F2
#define ATA_PRIMARY_LBA_LO 0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HI 0x1F5
#define ATA_PRIMARY_DRIVE_SEL 0x1F6
#define ATA_PRIMARY_STATUS 0x1F7
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_PRIMARY_ALT_STATUS 0x3F6
// bits of status register
#define ATA_SR_BSY 0x80 //busy
#define ATA_SR_DRDY 0x40 //drive ready
#define ATA_SR_DRQ 0x08 //data request
#define ATA_SR_ERR 0x01 //error
//commands
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY 0xEC
//secondary ATA chanel
#define ATA_SECONDARY_DATA 0x170
#define ATA_SECONDARY_ERROR 0x171
#define ATA_SECONDARY_SECTOR_CNT 0x172
#define ATA_SECONDARY_LBA_LO 0x173
#define ATA_SECONDARY_LBA_MID 0x174
#define ATA_SECONDARY_LBA_HI 0x175
#define ATA_SECONDARY_DRIVE_SEL 0x176
#define ATA_SECONDARY_STATUS 0x177
#define ATA_SECONDARY_COMMAND 0x177
#define ATA_SECONDARY_ALT_STATUS 0x376
//result of identify
typedef struct{
	uint16_t flags;
	uint16_t unused1[9];
	char serial[20];
	uint16_t unused2[3];
	char firmware[8];
	char model[40];
	uint16_t unused3[13];
	uint32_t lba28_sectors; // count of sectors in LBA28
} AtaIdentify;
int ata_init();//0=drive found
int ata_read(uint32_t lba, uint8_t count, uint8_t* buf);//0=ok
int ata_write(uint32_t lba, uint8_t count, uint8_t* buf);//0=ok
uint32_t ata_sector_count();							 						
uint8_t ata_get_current_drive();
