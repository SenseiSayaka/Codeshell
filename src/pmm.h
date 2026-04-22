#pragma once
#include "stdint.h"
#define PAGE_SIZE 4096

void pmm_init(uint32_t mem_upper);
uint32_t pmm_alloc(); // return physical address of page
void pmm_free(uint32_t addr);
uint32_t pmm_used_pages();
uint32_t pmm_free_pages();
