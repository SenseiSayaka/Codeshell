#pragma once
#include "stdint.h"

// flags in mbi->flags
#define MB_FLAG_MEM  (1 << 0)
#define MB_FLAG_MODS (1 << 3)

typedef struct
{
	uint32_t mod_start;
	uint32_t mod_end;
	uint32_t cmdline;
	uint32_t reserved;

} MultibootModule;

typedef struct
{
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t syms[4];
	uint32_t mmap_length;
	uint32_t mmap_addr;
} MultibootInfo;
