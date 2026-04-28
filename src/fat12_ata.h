#pragma once
#include "stdint.h"
int fat12_ata_init();
int fat12_ata_ls(char names[][13],uint32_t* sizes,int max);
int fat12_ata_read(const char* filename,uint8_t* buf,uint32_t buf_size);
int fat12_ata_create(const char* filename,const uint8_t* data,uint32_t size);
int fat12_ata_delete(const char* filename);
