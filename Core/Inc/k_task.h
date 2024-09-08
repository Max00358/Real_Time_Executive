/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_

#include <stdio.h>
#include "common.h"

//#define SHPR2 *(uint32_t*)0xE000ED1C //for setting SVC priority, bits 31-24
//#define SHPR3 *(uint32_t*)0xE000ED20 //PendSV is bits 23-16
#define SHPR2 (*((volatile uint32_t*)0xE000ED1C))//SVC is bits 31-28
#define SHPR3 (*((volatile uint32_t*)0xE000ED20))//SysTick is bits 31-28, and PendSV is bits 23-20

typedef unsigned int U32;
typedef unsigned short U16;
typedef char U8;
typedef unsigned int task_t;

typedef struct task_control_block {
	void (*ptask)(void* args); //entry address
	U32 stack_high; //starting address of stack (high address)
	task_t tid; //task ID
	U8 state; //task's state
	U16 stack_size; //max stack size. Must be a multiple of 8
	//your own fields at the end
	uint32_t* p_chunk;

	int initial_deadline;
	int current_deadline;

    union {
		size_t MALLOC_size;
    	void* MALLOC_p_buf;

    	struct {
    		int deadline;
			struct TCB* p_new_task;
    	} CREATE_TASK_args;
    	int CREATE_TASK_status;

    	int SLEEP_timeInMs;

    	struct {
    		int deadline;
    		task_t TID;
    	} SET_DEADLINE_args;
    	int SET_DEADLINE_status;

    	char* PRINTF_p_str;

    	void* FREE_p_mem;
    	int FREE_status;
    } SVC;


	uint32_t* p_stack;
} TCB;

void test();
void osKernelInit(void);
int osCreateDeadlineTask_SVC(int deadline, TCB* task);
int osCreateDeadlineTask(int deadline, TCB* task);
int osCreateTask(TCB* task);
int osCreateTask(TCB* task);
int osKernelStart(void);
void osYield(void);
int osTaskInfo(task_t TID, TCB* task_copy);
task_t osGetTID(void);
void osSleep(int timeInMs);
void osSleep_SVC(int timeInMs);
void osPeriodYield(void);
void osPeriodYield_SVC(void);
int osSetDeadline(int deadline, task_t TID);
int osSetDeadline_SVC(int deadline, task_t TID);

int osTaskExit(void);
void osScheduler(void);


#endif /* INC_K_TASK_H_ */
