#pragma once
#include "stdint.h"
//ELF magic
#define ELF_MAGIC 0x464C457F
//type of segments
#define PT_LOAD 1
//ELF header
typedef struct{
	uint32_t magic;
	uint8_t bits;
	uint8_t endian;
	uint8_t version1;
	uint8_t abi;
	uint8_t padding[8];
	uint16_t type;
	uint16_t machine;
	uint32_t version2;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} __attribute__((packed)) ElfHeader;
//Program header
typedef struct{
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
} __attribute__((packed)) ElfProgramHeader;
typedef struct{
	uint32_t entry;
	uint32_t page_dir;
	int error;
} ElfLoadResult;
ElfLoadResult elf_load(const uint8_t* data, uint32_t size);
void elf_copy_to_user(uint32_t dir_phys, uint32_t virt,
		const uint8_t* src, uint32_t len);
void elf_zero_user(uint32_t dir_phys, uint32_t virt, uint32_t len);
