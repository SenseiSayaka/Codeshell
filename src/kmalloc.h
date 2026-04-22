#pragma once
#include "stdint.h"

void heap_init(uint32_t base);
void* k_malloc(uint32_t size);
void k_free(void* ptr);
void* k_realloc(void* ptr, uint32_t new_size);

uint32_t heap_used();
uint32_t heap_free();
int heap_check();

//debug
void heap_dump();
