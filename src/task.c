#include "task.h"
#include "kmalloc.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "paging.h"
#include "pmm.h"
#include "gdt.h"
#include "vga.h"
static Task tasks[MAX_TASKS];
static int task_count=0;
static int current_task=0;
static uint8_t scheduler_ready=0;
static uint8_t scheduler_locked=0;
void scheduler_lock(){ scheduler_locked=1; }
void scheduler_unlock(){ scheduler_locked=0; }
extern void task_switch_asm(uint32_t* old_esp, uint32_t new_esp);
static void(*task_entry_points[MAX_TASKS])()={0};
//user stack
#define USER_STACK_VIRT 0xBFFFF000
#define USER_STACK_SIZE 4096
void tasks_init(){
	memset(tasks, 0, sizeof(tasks));
	//task 0 - it's kernel, alredy running
	// kernel's stack in boot.s
	tasks[0].state = TASK_RUNNING;
	tasks[0].stack = 0;
	uint32_t i = 0;
	while("kernel"[i] && i<31){
		tasks[0].name[i]="kernel"[i];
		i++;
	}
	tasks[0].name[i]='\0';
	task_count=1;
	current_task=0;
	scheduler_ready=1;
}
static void task_trampoline(){
	asm volatile("sti");
	int id=current_task;
	task_entry_points[id]();
	task_exit();
}
int task_create(const char* name, void (*entry)()){
	if(task_count >= MAX_TASKS) return -1;
	uint32_t* stack = (uint32_t*) k_malloc(TASK_STACK_SIZE);
	if(!stack) return -1;
	int id=task_count;
	task_entry_points[id]=entry;
	//top of the stack - elder byte
	uint32_t* sp = stack + TASK_STACK_SIZE / 4;
	// init frame, imitatate task_switch_asm
	// sequence: ret_Addr, ebp, ebx, esi, edi(bot to top)
	*(--sp)=(uint32_t)task_trampoline;//ret
	*(--sp)=0;//ebp
	*(--sp)=0;//ebx
	*(--sp)=0;//esi
	*(--sp)=0;//edi - init esp
	tasks[id].esp=(uint32_t)sp;
	tasks[id].stack=stack;
	tasks[id].state=TASK_READY;
	uint32_t i=0;
	while(name[i] && i < 31){
		tasks[id].name[i]=name[i];
		i++;
	}
	tasks[id].name[i] = '\0';
	task_count++;
	return id;
}
void schedule(){
	if(!scheduler_ready)return;
	if(scheduler_locked)return;
	if(task_count <2) return;
	//find next task
	int next = -1;
	for(int i=1;i<=task_count;i++){
		int candidate = (current_task+i)%task_count;
		if(candidate==current_task)continue;
		if(tasks[candidate].state==TASK_READY ||
				tasks[candidate].state==TASK_RUNNING){
			next=candidate;
			break;
		}
	}
	if(next==-1||next==current_task) return;
	int prev=current_task;
	if(tasks[prev].state==TASK_RUNNING)
        tasks[prev].state=TASK_READY; 
	tasks[next].state=TASK_RUNNING;
	current_task=next;
	if(tasks[next].stack != 0) {
        	tss_set_kernel_stack((uint32_t)tasks[next].stack+TASK_STACK_SIZE);
    	}
	if(tasks[next].page_dir!=0){
		paging_switch(tasks[next].page_dir);
	}else{
		extern uint32_t kernel_dir_phys;
		paging_switch(kernel_dir_phys);
	}
	task_switch_asm(&tasks[prev].esp, tasks[next].esp);
}
int task_create_user(const char* name,uint32_t entry,uint32_t page_dir){
	if(task_count>=MAX_TASKS)return -1;
	//allocate physical page to user stack
	uint32_t stack_phys=pmm_alloc();
	if(!stack_phys)return -1;
	paging_map(page_dir,
			USER_STACK_VIRT, stack_phys,
			PAGE_PRESENT|PAGE_WRITABLE|PAGE_USER);
	//kernel stack to process interrupts/syscall
	uint32_t* kstack=(uint32_t*)k_malloc(TASK_STACK_SIZE);
	if(!kstack){ pmm_free(stack_phys); return -1; }
	//init frame for IRET in ring3
	//[ss][esp][eflags][cs][eip]
	uint32_t* sp=kstack+TASK_STACK_SIZE/4;
	*(--sp)=0x23;
	*(--sp)=USER_STACK_VIRT+4092;
	*(--sp)=0x202;
	*(--sp)=0x1B;
	*(--sp)=entry;
	//then callee-szved for task_switch_asm
	extern void task_usermode_enter();
	*(--sp)=(uint32_t)task_usermode_enter;
	*(--sp)=0;
	*(--sp)=0;
	*(--sp)=0;
	*(--sp)=0;
	int id=task_count++;
	tasks[id].esp=(uint32_t)sp;
	tasks[id].stack=kstack;
	tasks[id].state=TASK_READY;
	tasks[id].page_dir=page_dir;
	uint32_t i=0;
	while(name[i]&&i<31){ tasks[id].name[i]=name[i];i++; }
	tasks[id].name[i]='\0';
	return id;
}
void task_exit(){
	int id=current_task;
	tasks[id].state=TASK_DEAD;
	schedule();
	while(1) asm volatile("hlt");
}
int get_current_task_id(){
	return current_task;
}
void task_block(int task_id){
	if(task_id<0||task_id>=task_count)return;
	tasks[task_id].state=TASK_BLOCKED;
	if(task_id==current_task){
		schedule();
	}
}
void task_unblock(int task_id){
	if(task_id<0||task_id>=task_count)return;
	if(tasks[task_id].state==TASK_BLOCKED){
		tasks[task_id].state=TASK_READY;
	}
}
int task_get_count(){
	return task_count;
}
int task_get_info(int idx, char* name_out, uint8_t* state_out){
	if(idx<0||idx>=task_count)return -1;
	int i=0;
	while(tasks[idx].name[i]&&i<31){
		name_out[i]=tasks[idx].name[i];
		i++;
	}name_out[i]='\0';
	*state_out=tasks[idx].state;
	return 0;
}
