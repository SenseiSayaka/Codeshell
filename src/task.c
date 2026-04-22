#include "task.h"
#include "kmalloc.h"
#include "util.h"
#include "stdlib/stdio.h"

static Task tasks[MAX_TASKS];
static int task_count=0;
static int current_task=0;
static uint8_t scheduler_ready=0;
static uint8_t scheduler_locked=0;
void scheduler_lock(){ scheduler_locked=1; }
void scheduler_unlock(){ scheduler_locked=0; }
extern void task_switch_asm(uint32_t* old_esp, uint32_t new_esp);
static void(*task_entry_points[MAX_TASKS])()={0};
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
		if(tasks[candidate].state==TASK_READY ||
				tasks[candidate].state==TASK_RUNNING){
			next=candidate;
			break;
		}
	}
	if(next==-1||next==current_task) return;
	int prev=current_task;
	tasks[prev].state=TASK_READY;
	tasks[next].state=TASK_RUNNING;
	current_task=next;
	task_switch_asm(&tasks[prev].esp,tasks[next].esp);
}
void task_exit(){
	tasks[current_task].state=TASK_DEAD;
	schedule();
	for(;;);
}

