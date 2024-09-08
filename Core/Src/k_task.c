#include "common.h"
#include "k_task.h"
#include "k_mem.h"
#include <stdio.h>
#include "stm32f4xx_hal.h"


TCB tcb_array[MAX_TASKS];
int kernel_init = 0;
int kernel_running = 0;
int task_count = 0;


int running_task_index = -1;
int last_running_task_index = -1;

void NULL_TASK(void* args) {
//	printf("0\r\n");
//	SVC_printf(_NEUTRAL_ "NULL_TASK()\r\n");
	while (1) {
//		printf(_NEUTRAL_ "NULL_TASK()\r\n");
	}
}

int osCreateTask(TCB* p_new_task);

void SVC_printf(char* p_str) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.PRINTF_p_str = p_str;
	__SVC(SVC_PRINTF);
}

void osKernelInit(void) {
	if (kernel_init) { return; }

//	SHPR3 |= 0xFFU << 24; //Set the priority of SysTick to be the weakest
//	SHPR3 |= 0xFEU << 16; //shift the constant 0xFE 16 bits to set PendSV priority
//	SHPR2 |= 0xFDU << 24; //set the priority of SVC higher than PendSV
	SHPR3 = (SHPR3 & ~(0xFFU << 24)) | (0xF0U << 24);//SysTick is lowest priority (highest number)
	SHPR3 = (SHPR3 & ~(0xFFU << 16)) | (0xE0U << 16);//PendSV is in the middle
	SHPR2 = (SHPR2 & ~(0xFFU << 24)) | (0xD0U << 24);//SVC is highest priority (lowest number)

    task_count = 0;
    kernel_running = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        tcb_array[i].ptask = NULL;
        tcb_array[i].stack_high = -1;
        tcb_array[i].p_chunk = NULL;
        tcb_array[i].tid = -1;
        tcb_array[i].state = DORMANT;
        tcb_array[i].stack_size = 0x0;
        tcb_array[i].initial_deadline = 0;
        tcb_array[i].current_deadline = 0;
        // Initialize any additional fields here
    	tcb_array[i].p_stack = NULL;
    }

    kernel_init = 1;
    if (k_mem_init() == RTX_ERR) {
    	kernel_init = 0;
    	return;
    }

    TCB null_task = { .ptask = NULL_TASK, .stack_size = STACK_SIZE };
    osCreateDeadlineTask_SVC(INT_MAX, &null_task);

//    printf(_SUCCESS_ "osKernelInit()\r\n");
}

int osCreateTask(TCB* p_new_task) { return osCreateDeadlineTask(5, p_new_task); }

int osCreateDeadlineTask(int deadline, TCB* p_new_task) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.CREATE_TASK_args.deadline = deadline;
	p_task->SVC.CREATE_TASK_args.p_new_task = p_new_task;
	__SVC(SVC_CREATE_TASK);

//    printf(_SUCCESS_ "osCreateTask()\r\n");
	return p_task->SVC.CREATE_TASK_status;
}

int osCreateDeadlineTask_SVC(int deadline, TCB* p_new_task) {
	if (
		deadline <= 0 ||
		!p_new_task ||
		p_new_task->stack_size < STACK_SIZE ||
		!p_new_task->ptask ||
		task_count >= MAX_TASKS ||
		!kernel_init
	) { return RTX_ERR; }

	int i = 0;
	TCB* p_kernel_task = NULL;
	for (i = 0; i < MAX_TASKS; i++) {
		if (tcb_array[i].state == DORMANT) {
			p_kernel_task = &tcb_array[i];
			break;
		}
	}
	if (!p_kernel_task) { return RTX_ERR; }

	Block_Header* p_chunk = k_mem_alloc_SVC(p_new_task->stack_size);
	if (!p_chunk) { return RTX_ERR; }

	p_chunk = (Block_Header*)((uint32_t)p_chunk - (uint32_t)(sizeof(Block_Header)));
	uint32_t* p_stack = (uint32_t*)((uint32_t)p_chunk + (uint32_t)p_chunk->full_size);

	p_chunk->TID = i; // transfer memory ownership from calling task to created task

	p_new_task->tid = i;
	p_new_task->state = READY;
	p_new_task->initial_deadline = deadline;
	p_new_task->current_deadline = deadline;

	p_kernel_task->ptask = p_new_task->ptask;

	p_kernel_task->stack_high = (int)p_stack;
	p_kernel_task->p_chunk = (uint32_t*)p_chunk;
	p_kernel_task->tid = i;
	p_kernel_task->state = READY;
	p_kernel_task->stack_size = p_new_task->stack_size;

	*(--p_stack) = 1<<24;
	*(--p_stack) = (uint32_t)p_new_task->ptask;
	*(--p_stack) = (uint32_t)osTaskExit;
	p_stack -= 13;
	p_kernel_task->p_stack = p_stack;
	p_kernel_task->initial_deadline = deadline;
	p_kernel_task->current_deadline = deadline;

	++task_count;

	int current_task = osGetTID();
	TCB* p_current_task = &tcb_array[current_task];
//	printf(_SUCCESS_ "TC\r\n");

	if (
			kernel_running &&
			current_task > 0 &&
			deadline < p_current_task->current_deadline // <=
	) {
		p_current_task->state = READY;
		SCB->ICSR |= 1<<28;
		__asm("ISB");
	}

    return RTX_OK;
}

int osKernelStart(void) {
	if (
			!kernel_init ||
			kernel_running
	) { return RTX_ERR; }

	TCB* p_task = &tcb_array[1];
	if (p_task->state != READY) {
		p_task = &tcb_array[0];
		return RTX_ERR;
	}
	int index = 1;
	for (int i = 2; i < MAX_TASKS; i++) {
		if (
				tcb_array[i].state == READY &&
				tcb_array[i].current_deadline < p_task->current_deadline)
		{
			p_task = &tcb_array[i];
			index = i;
		}
	}

	__set_PSP((int)p_task->p_stack);
	p_task->state = RUNNING;

	last_running_task_index = index;
	running_task_index = index;
	kernel_running = 1;

	__SVC(SVC_CONTEXT_INIT);
	return RTX_OK;
}

int osTaskInfo(task_t TID, TCB* p_task_copy) {
	if (
			!p_task_copy ||
			TID < 1 ||
			TID >= MAX_TASKS ||
			!kernel_init ||
			tcb_array[TID].state == DORMANT ||
			TID != tcb_array[TID].tid
	) { return RTX_ERR; }

	p_task_copy->ptask = tcb_array[TID].ptask;
	p_task_copy->stack_high = tcb_array[TID].stack_high;
	p_task_copy->p_chunk = tcb_array[TID].p_chunk;
	p_task_copy->tid = tcb_array[TID].tid;
	p_task_copy->state= tcb_array[TID].state;
	p_task_copy->stack_size = tcb_array[TID].stack_size;
	p_task_copy->p_stack = tcb_array[TID].p_stack;
	p_task_copy->initial_deadline = tcb_array[TID].initial_deadline;
	p_task_copy->current_deadline = tcb_array[TID].current_deadline;

	return RTX_OK;
}

task_t osGetTID(void) {
    if (
    		kernel_running &&
			running_task_index >= 0 &&
			tcb_array[running_task_index].state == RUNNING
	) { return tcb_array[running_task_index].tid; }

    return 0;
}

void osYield(void) {
	__SVC(SVC_YIELD);
}

void osYield_SVC(void) {
	if (running_task_index < 0) { return; }

	TCB* p_task = &tcb_array[osGetTID()];
	p_task->state = READY;
	p_task->current_deadline = p_task->initial_deadline;

	last_running_task_index = running_task_index;
	running_task_index = -1;

	SCB->ICSR |= 1<<28;
	__asm("ISB");
}

int osTaskExit(void) {
	__SVC(SVC_TASK_EXIT);

//	printf(_FAILURE_ "x\r\n");
	return RTX_ERR; // always errors on return
}

void osSleep(int timeInMs) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.SLEEP_timeInMs = timeInMs;
	__SVC(SVC_SLEEP);
}

void osSleep_SVC(int timeInMs) {
	if (running_task_index < 0) { return; }

	TCB* p_task = &tcb_array[osGetTID()];
	p_task->state = SLEEPING;
	p_task->current_deadline = timeInMs;

	last_running_task_index = running_task_index;
	running_task_index = -1;

	SCB->ICSR |= 1<<28;
	__asm("ISB");
}

void osPeriodYield(void) {
	__SVC(SVC_PERIOD_YIELD);
}

void osPeriodYield_SVC(void) {
	if (running_task_index < 0) { return; }

	TCB* p_task = &tcb_array[osGetTID()];
	p_task->state = SLEEPING;

	last_running_task_index = running_task_index;
	running_task_index = -1;

	SCB->ICSR |= 1<<28;
	__asm("ISB");
}

int osSetDeadline(int deadline, task_t TID) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.SET_DEADLINE_args.deadline = deadline;
	p_task->SVC.SET_DEADLINE_args.TID = TID;
	__SVC(SVC_SET_DEADLINE);

	return p_task->SVC.SET_DEADLINE_status;
}

int osSetDeadline_SVC(int deadline, task_t TID) {
	task_t curr_TID = osGetTID();
	TCB* p_curr_task = &tcb_array[curr_TID];
	if (
			deadline <= 0 ||
			TID == curr_TID ||
			TID <= 0 ||
			TID >= MAX_TASKS ||
			tcb_array[TID].state == DORMANT
	) { p_curr_task->SVC.SET_DEADLINE_status = RTX_ERR; }

	TCB* p_target_task = &tcb_array[TID];
	p_target_task->initial_deadline = deadline;
	p_target_task->current_deadline = deadline;


	p_curr_task->SVC.SET_DEADLINE_status = RTX_OK;
	if (deadline <= p_curr_task->current_deadline) {
		p_curr_task->state = READY;
		last_running_task_index = running_task_index;
		running_task_index = -1;

		SCB->ICSR |= 1<<28;
		__asm("ISB");
	}
}


//int osTaskExit_SVC(void) {
//	if (
//			running_task_index < 0 ||
//			tcb_array[running_task_index].state == DORMANT
//	) { return RTX_ERR; }
//
//	TCB* p_task = &tcb_array[running_task_index];
//
//	void* p_mem_block = (void*)((uint32_t)p_task->p_chunk + (uint32_t)sizeof(Block_Header));
//	if (k_mem_dealloc(p_mem_block) == RTX_ERR) {
//		printf("task free failed\r\n");
//		return RTX_ERR;
//	}
//
//	p_task->state = DORMANT;
//
//	last_running_task_index = running_task_index;
//	running_task_index = -1;
//	--task_count;
//
//	__asm("SVC #2");
//
//	return RTX_OK; // never reached
//}

void osScheduler(void) {
	if (last_running_task_index >= 0) {
		TCB* p_last_task = &tcb_array[last_running_task_index];
		p_last_task->p_stack = (uint32_t*)__get_PSP();
	}

	TCB* p_task = &tcb_array[0];
	int min_deadline = INT_MAX;

	for (int i = 1; i < MAX_TASKS; i++) {
		if (
				tcb_array[i].state == READY &&
				tcb_array[i].current_deadline < min_deadline
		) {
			p_task = &tcb_array[i];
			min_deadline = p_task->current_deadline;
		}
	}

	p_task->state = RUNNING;
	running_task_index = p_task->tid;
	__set_PSP((int)p_task->p_stack);
}










