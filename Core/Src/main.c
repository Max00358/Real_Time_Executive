/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "main.h"
#include "k_task.h"
#include <stdio.h> //You are permitted to use this library, but currently only printf is implemented. Anything else is up to you!

/**
  * @brief  The application entry point.
  * @retval int
  */


/*
 * Registers: xPSR, PC, LR, etc
 * Thread Registers:
 *
	push our registers to our stack
	push registers we want to the thread stack
	change SP to thread stack
	pop each register from the TS to the processor
	Continue execution

	When returning back to MS
	pop all the registers back to the processor

thread stack{
	push main registers to our stack
	push thread registers to thread
	change stack pointer to thread
	pop all the thread registers into the processor
	continue execution

	when returning to main stack
	pop all the registers into the processor
	continue execution
}
	before leaving save processor context to stack
	SP = 0x40
	*0x40 = xPSR
	*0x44 = PC

	upon return load registers back into processor
	PC = *0x40
	...
	*/
// for when they replace our main file
extern TCB tcb_array[MAX_TASKS];
extern int kernel_running;
extern int current_task_index;

void f_task_1(void* args);

void f_recursive_task(void* args) {
	TCB task = { .ptask = f_recursive_task, .stack_size = 0x400 };
	int status = osCreateTask(&task);
	printf("osCreateTask(): %d\r\n", status);
	for (int i = 0; i < MAX_TASKS; i++) {
		printf("Task %d State: %d at %p, p_stack %p\r\n", i, tcb_array[i].state, &tcb_array[i], tcb_array[i].p_stack);
	}

	osTaskExit();
}
void f_task_1(void* args) {
	printf("task 1\r\n");
	while (1) {
		osYield();
	}
	osTaskExit();
}
void f_task_2(void* args) {
	printf("task 2\r\n");
	while (1) {
		osYield();
	}
	osTaskExit();
}
void f_task_3(void* args) {
	printf("task 3\r\n");
	while (1) {
		osYield();
	}
	osTaskExit();
}
void f_task_4(void* args) {
	printf("task 4\r\n");
	while (1) {
		osYield();
	}
	osTaskExit();
}

//3 tasks of same deadline of 4 and task b changes deadline of task c to be 2 ms
//osPeriod yeild after every loop
void f_task_A(void* args) {
	for (int i = 0; i < 5; i++) {
		printf("A\r\n");
		osPeriodYield();
	}
}
void f_task_B(void* args) {
	for (int i = 0; i < 5; i++) {
		if (i == 2) {
			printf("set C 2ms\r\n");
			osSetDeadline(2, 3);
		}
		printf("B\r\n");
		osPeriodYield();
	}
}
void f_task_C(void* args) {
	for (int i = 0; i < 8; i++) {
		printf("C\r\n");
		osPeriodYield();
	}
}

void Set_Up_Thread_Stack_and_Context(void (*p_func)(void)) {
	uint32_t* MSP_INIT_VAL = *(uint32_t**)0x0;
	uint32_t* p_stack = (uint32_t*)((int)MSP_INIT_VAL - (int)0x400); //bottom address of stack

	*(--p_stack) = 1<<24; // (xPSR) setting the chip to Thumb mode
	*(--p_stack) = (uint32_t)p_func; // (PC) the function name
	*(--p_stack) = (uint32_t)osTaskExit; // (LR)
	for (int i = 0; i < 13; i++) {
		*(--p_stack) = 10*(i + 1);
	}
	__set_PSP((int)p_stack);
	//__asm volatile("MSR PSP, %0" :: "r"(p_stack));

	printf("Before SVC #1\r\n");
	__asm("SVC #1");
}

int count = 0;
//#define MALLOC_SVC 9
//#define STR(x) #x
//#define __SVC(num) __asm("SVC #" STR(num))


int _________main(void) {
	/* MCU Configuration: Don't change this or the whole chip won't work!*/
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	/* MCU Configuration is now complete. Start writing your code below this line */
	printf("\e[1;1H\e[2J\r\n\r\n\r\n");
	printf("-------New----------\r\n");

	osKernelInit();

//	TCB task_1 = { .ptask = f_task_1, .stack_size = 0x400 };
//	TCB task_2 = { .ptask = f_task_2, .stack_size = 0x400 };
//	TCB task_3 = { .ptask = f_task_3, .stack_size = 0x400 };
//	TCB task_4 = { .ptask = f_task_4, .stack_size = 0x400 };

//	TCB task_4 = { .ptask = f_recursive_task, .stack_size = 0x400 };
//	TCB task_5 = { .ptask = f_recursive_task, .stack_size = 0x400 };

	TCB task_A = { .ptask = f_task_A, .stack_size = 0x400 };
	TCB task_B = { .ptask = f_task_B, .stack_size = 0x400 };
	TCB task_C = { .ptask = f_task_C, .stack_size = 0x400 };

	osCreateDeadlineTask(4, &task_A);
	osCreateDeadlineTask(4, &task_B);
	osCreateDeadlineTask(4, &task_C);


//	osCreateDeadlineTask(30, &task_1);
//	osCreateDeadlineTask(50, &task_2);
//	osCreateDeadlineTask(10, &task_3);
//	osCreateDeadlineTask(5, &task_4);
//	osCreateTask(&task_5);
//
	osKernelStart();
}
