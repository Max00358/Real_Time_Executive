/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "k_mem.h"
#include "k_task.h"
#include "main.h"
#include "stm32f4xx_it.h"
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern int count;

extern TCB tcb_array[];
extern int kernel_running;

extern int last_running_task_index;
extern int running_task_index;
extern int task_count;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {
	/* USER CODE BEGIN NonMaskableInt_IRQn 0 */
	printf("NMI_Handler\r\n");
	/* USER CODE END NonMaskableInt_IRQn 0 */
	/* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1) {

	}
	/* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void) {
	/* USER CODE BEGIN HardFault_IRQn 0 */
	printf("HardFault_Handler\r\n");
	/* USER CODE END HardFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_HardFault_IRQn 0 */
		/* USER CODE END W1_HardFault_IRQn 0 */
	}
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void) {
	/* USER CODE BEGIN MemoryManagement_IRQn 0 */
	printf("MemManage_Handler\r\n");
	/* USER CODE END MemoryManagement_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
		/* USER CODE END W1_MemoryManagement_IRQn 0 */
	}
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void) {
	/* USER CODE BEGIN BusFault_IRQn 0 */
	printf("BusFault_Handler\r\n");
	/* USER CODE END BusFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_BusFault_IRQn 0 */
		/* USER CODE END W1_BusFault_IRQn 0 */
	}
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void) {
	/* USER CODE BEGIN UsageFault_IRQn 0 */
	printf("UsageFault_Handler\r\n");
	/* USER CODE END UsageFault_IRQn 0 */
	while (1) {
		/* USER CODE BEGIN W1_UsageFault_IRQn 0 */
		/* USER CODE END W1_UsageFault_IRQn 0 */
	}
}

/**
  * @brief This function handles System service call via SWI instruction.
  */




extern void SVC_Handler_Main(unsigned int* svc_args) {
	/* Stack contains:
	* r0, r1, r2, r3, r12, r14, the return address and xPSR
	* First argument (r0) is svc_args[0]*/
	unsigned int svc_number = ((char*)svc_args[6])[-2];

	TCB* p_task = NULL;
	int deadline = 0;

	switch (svc_number) {
		case SVC_ENABLE_PRIVILEGEDMODE:  /* EnablePrivilegedMode */
			__set_CONTROL(__get_CONTROL() & ~CONTROL_nPRIV_Msk);
			break;
		case SVC_CONTEXT_INIT:
			SysTick->VAL = 0;
			__asm__ volatile(
					"MRS r0, PSP\n\t"
					"LDMIA r0!, {r4-r11}\n\t"
					"MSR PSP, r0 \n\t"
					"MOV LR, #0xFFFFFFFD\n\t"
					"BX LR\n\t"
			);
			break;

		case SVC_CONTEXT_SWITCH:
			SCB->ICSR |= 1<<28;
			__asm("ISB");
			break;

		case SVC_MALLOC:
			p_task = &tcb_array[osGetTID()];
			size_t size = p_task->SVC.MALLOC_size;
			p_task->SVC.MALLOC_p_buf = (void*)k_mem_alloc_SVC(size);
			break;
		case SVC_FREE:
			p_task = &tcb_array[osGetTID()];
	    	void* p_mem = p_task->SVC.FREE_p_mem;
	    	p_task->SVC.FREE_status = (int)k_mem_dealloc_SVC(p_mem);
			break;
		case SVC_YIELD:
			osYield_SVC();
			break;

		case SVC_TASK_EXIT:
			p_task = &tcb_array[osGetTID()];

			if (
					running_task_index < 0 ||
					p_task->state == DORMANT
			) { break; }

			void* p_mem_block = (void*)((uint32_t)p_task->p_chunk + (uint32_t)sizeof(Block_Header));
			if (k_mem_dealloc_SVC(p_mem_block) == RTX_ERR) { break; }

			p_task->state = DORMANT;

			last_running_task_index = running_task_index;
			running_task_index = -1;
			--task_count;

			SCB->ICSR |= 1<<28;
			__asm("ISB");
			break;

		case SVC_CREATE_TASK:
			p_task = &tcb_array[osGetTID()];

			deadline = p_task->SVC.CREATE_TASK_args.deadline;
			TCB* p_new_task = (TCB*)p_task->SVC.CREATE_TASK_args.p_new_task;

			p_task->SVC.CREATE_TASK_status = osCreateDeadlineTask_SVC(deadline, p_new_task);
			break;

		case SVC_SLEEP:
			p_task = &tcb_array[osGetTID()];

			osSleep_SVC(p_task->SVC.SLEEP_timeInMs);
			break;

		case SVC_PERIOD_YIELD:
			osPeriodYield_SVC();
			break;

		case SVC_SET_DEADLINE:
			p_task = &tcb_array[osGetTID()];

			deadline = p_task->SVC.SET_DEADLINE_args.deadline;
			task_t TID = p_task->SVC.SET_DEADLINE_args.TID;

			p_task->SVC.SET_DEADLINE_status = osSetDeadline_SVC(deadline, TID);
			break;

		case SVC_PRINTF:
			p_task = &tcb_array[osGetTID()];

			printf("%s", p_task->SVC.PRINTF_p_str);
			break;

		case SVC_TEST:
			break;

		default: break;    /* unknown SVC */
	}

	return;
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void) {
	/* USER CODE BEGIN DebugMonitor_IRQn 0 */
	printf("DebugMon_Handler\r\n");
	/* USER CODE END DebugMonitor_IRQn 0 */
//	__asm__ volatile(
//
//	);
	/* USER CODE BEGIN DebugMonitor_IRQn 1 */
	/* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
//void PendSV_Handler(void) {
//	/* USER CODE BEGIN PendSV_IRQn 0 */
//
//	/* USER CODE END PendSV_IRQn 0 */
//	printf("PendSV Called\r\n");
//	/* USER CODE BEGIN PendSV_IRQn 1 */
//
//	/* USER CODE END PendSV_IRQn 1 */
//}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void) {
	/* USER CODE BEGIN SysTick_IRQn 0 */
	/* USER CODE END SysTick_IRQn 0 */
	HAL_IncTick();
	/* USER CODE BEGIN SysTick_IRQn 1 */
	if (!kernel_running) { return; }

	TCB* p_task = NULL;
	int TID = osGetTID();
	TCB* p_current_task = &tcb_array[TID];
	int trigger = 0;
	if (TID == 0) { trigger = 1; }

	for (int i = 1; i < MAX_TASKS; i++) {
		p_task = &tcb_array[i];
		if (p_task->state == DORMANT) { continue; }

		if (--p_task->current_deadline == 0) {
			p_task->current_deadline = p_task->initial_deadline;

			if (p_task->state == RUNNING) {
				trigger = 1;
			} else if (p_task->state == READY) {
				// Ask Maran, should we trigger
			} else if (p_task->state == SLEEPING) {
				p_task->state = READY; // Ask Maran, should it be in the scheduler
				if (p_task->current_deadline <= p_current_task->current_deadline) {
					trigger = 1; // Ask Maran, what happens if deadline was 0 and got reset?
				}
			}
		}
	}


	if (trigger) {
		p_current_task->state = READY;

		last_running_task_index = running_task_index;
		running_task_index = -1;

		SCB->ICSR |= 1<<28;
		__asm("ISB");
	}

	return;
	/* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
