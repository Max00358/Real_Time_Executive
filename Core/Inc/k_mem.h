/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_

#include "common.h"
#include "k_task.h"

#define NUM_LAYERS 11
#define MAGIC_NUM 47282010

typedef struct __Block_Header { // Check names before submitting
	int free;
	int TID;
	int free_list_index;
	int magic_num;

	size_t full_size; // full_size = data_size + size(Block_Header);

	struct __Block_Header* p_prev;
	struct __Block_Header* p_next;
} Block_Header;


int k_mem_init();
void* k_mem_alloc(size_t size);
int k_mem_dealloc(void* p_mem);
int k_mem_count_extfrag(size_t size);

#endif /* INC_K_MEM_H_ */
