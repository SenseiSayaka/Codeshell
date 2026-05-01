#pragma once
#include "stdint.h"
#define TASK_RUNNING 0
#define TASK_READY 1
#define TASK_DEAD 2
#define TASK_BLOCKED 3
#define MAX_TASKS 16
#define TASK_STACK_SIZE 4096
typedef struct{
	uint32_t esp;
	uint32_t* stack;
	uint8_t state;
	char name[32];
	uint32_t page_dir;
} Task;
void tasks_init();
int task_create(const char* name, void(*entry)());
void schedule();
void task_exit();
void scheduler_lock();
void scheduler_unlock();
int get_current_task_id();
int task_create_user(const char* name,uint32_t entry,uint32_t page_dir);
void task_block(int task_id);
void task_unlock(int task_id);
void tss_set_kernel_stack(uint32_t esp0);
int task_get_count();
int task_get_info(int idx, char* name_out, uint8_t* state_out);

