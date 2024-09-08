/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */
#include <limits.h>
#include <stdint.h>

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#define true 1
#define false 0

#define __SVC(num) __asm("SVC #" STR(num))
#define STR(x) #x
#define SVC_ENABLE_PRIVILEGEDMODE 0
#define SVC_CONTEXT_INIT 1
#define SVC_CONTEXT_SWITCH 2
#define SVC_MALLOC 3
#define SVC_YIELD 4
#define SVC_TASK_EXIT 5
#define SVC_CREATE_TASK 6
#define SVC_SLEEP 7
#define SVC_PERIOD_YIELD 8
#define SVC_SET_DEADLINE 9
#define SVC_FREE 10
#define SVC_PRINTF 100
#define SVC_TEST 17

#define TID_NULL 0 //predefined Task ID for the NULL task
#define MAX_TASKS 16 //maximum number of tasks in the system
#define STACK_SIZE 0x200 //minimum. size of each task’s stack

#define DORMANT 0 //state of terminated task
#define READY 1 //state of task that can be scheduled but is not running
#define RUNNING 2 //state of running task
#define SLEEPING 3 // state of sleeping task

#define RTX_OK 0
#define RTX_ERR -1

#define _RESET_ "\033[0m"
#define _RED_ "\033[31m"
#define _YELLOW_ "\033[33m"
#define _GREEN_ "\033[32m"
#define _FAILURE_ "\033[31m[-] \033[0m"
#define _NEUTRAL_ "\033[33m[•] \033[0m"
#define _SUCCESS_ "\033[32m[+] \033[0m"



#endif /* INC_COMMON_H_ */
