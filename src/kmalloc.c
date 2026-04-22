#include "kmalloc.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "task.h"
// config
#define HEAP_SIZE (1024 * 1024)
#define MIN_SPLIT 16
#define CANARY_VALUE 0xDEADBEEF // magic number

typedef struct Block
{
	uint32_t canary_head;
	uint32_t size;
	uint8_t free;
	struct Block* next;
	uint32_t canary_tail;
} Block;

#define BLOCK_HDR sizeof(Block)

extern uint32_t kernel_end;

static Block* heap_start = 0;

// init
void heap_init(uint32_t base)
{
    
    base = (base + 3) & ~3;
    heap_start       = (Block*) base;
    heap_start->canary_head = CANARY_VALUE;
    heap_start->canary_tail = CANARY_VALUE;
    heap_start->size = HEAP_SIZE - BLOCK_HDR;
    heap_start->free = 1;
    heap_start->next = 0;
}
//check one block
static int block_ok(Block* b){
	return b->canary_head==CANARY_VALUE &&
		b->canary_tail==CANARY_VALUE;
}
// merger neighbour blocks

static void coalesce()
{
	Block* cur = heap_start;
	while(cur && cur->next)
	{
		if(!block_ok(cur))return;
		if(cur->free && cur->next->free)
		{
			cur->size += BLOCK_HDR + cur->next->size;
			cur->next = cur->next->next;
		}
		else
		{
			cur = cur->next;
		}
	}
}
// allocate memory
void* k_malloc(uint32_t size) {
    scheduler_lock();
    if (!heap_start||size == 0){ scheduler_unlock(); return 0; }
    size = (size + 3) & ~3;
    Block* cur = heap_start;
    while (cur) {
	    if(!block_ok(cur)){
		    printf("k_malloc: heap corruption detected.\n");
		    scheduler_unlock();
		    return 0;
	    }
        if (cur->free && cur->size >= size) {
            if (cur->size >= size + BLOCK_HDR + MIN_SPLIT) {
                Block* newBlock = (Block*)((uint8_t*)cur + BLOCK_HDR + size);
		newBlock->canary_head=CANARY_VALUE;
		newBlock->canary_tail=CANARY_VALUE;
                newBlock->size = cur->size - size - BLOCK_HDR;
                newBlock->free = 1;
                newBlock->next = cur->next;
                cur->size = size;
                cur->next = newBlock;
            }
            cur->free = 0;
	    	scheduler_unlock();
            return (void*)((uint8_t*)cur + BLOCK_HDR);
        }
        cur = cur->next;
    }
    printf("k_malloc: out of memory.\n");
    scheduler_unlock();
    return 0;
}
//free memory
void k_free(void* ptr)
{
	if(!ptr) return;
	scheduler_lock();
	Block* block = (Block*)((uint8_t*)ptr - BLOCK_HDR);
	if(!block_ok(block)){
		printf("k_free: heap corruption detected.\n");
		scheduler_unlock();
		return;
	}
	if(block->free){
		printf("k_free: double free detected.\n");
		scheduler_unlock();
		return;
	}
	block->free = 1;
	coalesce();
	scheduler_unlock();
}

// change size of memory

void* k_realloc(void* ptr, uint32_t new_size)
{
	if(!ptr) return k_malloc(new_size);
	if(!new_size)
	{
		k_free(ptr);
		return 0;
	}
	scheduler_lock();
	Block* block = (Block*)((uint8_t*)ptr - BLOCK_HDR);
	if(!block_ok(block)){
		printf("k_realloc: heap corruption detected.\n");
		scheduler_unlock();
		return 0;
	}
	if (block->size >= new_size){ scheduler_unlock(); return ptr; }
	scheduler_unlock();
	void* new_ptr = k_malloc(new_size);
	if(!new_ptr) return 0;
	
	uint8_t* src = (uint8_t*)ptr;
	uint8_t* dst = (uint8_t*)new_ptr;
	uint32_t n = block->size < new_size ? block->size : new_size;
	for (uint32_t i = 0; i < n; i++) dst[i] = src[i];
	k_free(ptr);
	return new_ptr;
}
// statistics
uint32_t heap_used(){
	uint32_t used=0;
	Block* cur=heap_start;
	while(cur){
		if(!block_ok(cur))break;
		if(!cur->free)used+=cur->size;
		cur=cur->next;
	}
	return used;
}
uint32_t heap_free(){
	uint32_t free=0;
	Block* cur=heap_start;
	while(cur){
		if(!block_ok(cur))break;
		if(cur->free)free+=cur->size;
		cur=cur->next;
	}
	return free;
}
// integrity check
int heap_check(){
	Block* cur=heap_start;
	uint32_t count=0;
	while(cur){
		if(!block_ok(cur)){
			printf("heap_check: corruption at block %d (addr=0x%x)\n",
					count, (uint32_t)cur);
			return -1;
		}
		cur=cur->next;
		count++;
	}
	return 0;
}
// debug dump

void heap_dump()
{
	Block* cur = heap_start;
	uint32_t idx = 0;
	uint32_t used=heap_used();
	uint32_t free=heap_free();
	printf("\n-------------  heap dump  -------------\n");
	printf("\n---- heap: used=%d free=%d total=%d ----\n",
			used, free, used + free);
	while(cur)
	{
		if(!block_ok(cur)){
			printf("  [%d] CORRUPTED at 0x%x!\n", idx, (uint32_t)cur);
			break;
		}
		printf("  [%d] addr=0x%x  size=%d  %s\n",
				idx,
				(uint32_t)cur + BLOCK_HDR,
				cur->size,
				cur->free ? "FREE" :"USED");
		cur = cur->next;
		idx++;
	}
	printf("---------------------------------------\n");
}

