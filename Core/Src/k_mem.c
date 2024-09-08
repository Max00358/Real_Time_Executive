#include "k_mem.h"
#include "k_task.h"

uint32_t _img_end;
uint32_t _estack;
uint32_t _Min_Stack_Size;
uint32_t heap_end;

extern int kernel_init;
extern TCB tcb_array[MAX_TASKS];

int kernel_mem_init = 0;
int max_chunk_size = 32*1024;
int total_allocated_memory = 0;
Block_Header* free_list[NUM_LAYERS];

void print_free_list() {
	for (int i = 0; i < NUM_LAYERS; i++) {
		int count = 0;
		int size = 0;
		Block_Header* p_head = free_list[i];
		if (p_head) {
			size = p_head->full_size;
			printf("i: %d, Free: ", i);
		} else {
			printf("i: %d, Empty\r\n", i);
		}
		while (p_head != NULL) {
			count++;
			printf("%d @ %p,  ", p_head->full_size, p_head);
			p_head = p_head->p_next;
		}
		if (p_head || count) {
			printf("\r\n");
		}
	}
	printf("Exit\r\n");
}

int k_mem_init() {
	if (
			!kernel_init ||
			kernel_mem_init
	) { return RTX_ERR; }

	uint32_t curr_size = 32*1024;
	Block_Header* p_header = (Block_Header*)&_img_end;
	heap_end = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;

	for (int i = 0; i < NUM_LAYERS - 1; i++) {
		free_list[i] = NULL;
	}

	for (int i = NUM_LAYERS - 1; i >=0; i--) {
		if ((int)p_header + (int)curr_size <= (int)heap_end) {
			p_header->free = 1;
			p_header->TID = 0;
			p_header->free_list_index = i;
			p_header->magic_num = MAGIC_NUM;
			p_header->full_size = curr_size;
			p_header->p_prev = NULL;
			p_header->p_next = NULL;

			free_list[i] = p_header;
			p_header = (Block_Header*)((int)p_header + (int)curr_size);
		}
		curr_size >>= 1;
	}

	kernel_mem_init = 1;

//	printf(_SUCCESS_ "k_mem_init()\r\n");
	return RTX_OK;
}

void* k_mem_alloc(size_t size) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.MALLOC_size = size;
	__SVC(SVC_MALLOC);
// overwriting return value
	return p_task->SVC.MALLOC_p_buf;
}

void* k_mem_alloc_SVC(size_t size) {
	if (
			!kernel_mem_init ||
			size == 0 ||
			size > max_chunk_size - sizeof(Block_Header)
	) { return NULL; }

	int i = NUM_LAYERS - 1;
	int chunk_size = 32*1024;

	while (
			chunk_size >= (((uint32_t)((int)size + (int)sizeof(Block_Header))) << 1) &&
			i > 0
	) {
		--i;
		chunk_size >>= 1;
	}

	Block_Header* p_header = NULL;
	for (; i < NUM_LAYERS; i++) {
		if (free_list[i] && size <= free_list[i]->full_size - sizeof(Block_Header)) {
			p_header = free_list[i];
			free_list[i] = free_list[i]->p_next;
			if (free_list[i]) {
				free_list[i]->p_prev = NULL;
			}
			break;
		}
		chunk_size <<= 1;
	}
	if (!p_header) { return NULL; }

	while (
			chunk_size >> 1 >= (uint32_t)((int)size + (int)sizeof(Block_Header)) &&
			i > 0
	) {
		--i;
		chunk_size >>= 1;

		Block_Header* p_new_header = (Block_Header*)((int)p_header + (int)chunk_size); // check all cases
		p_new_header->free = 1;
		p_new_header->TID = 0;
		p_new_header->free_list_index = i;
		p_new_header->magic_num = MAGIC_NUM;
		p_new_header->full_size = chunk_size;
		p_new_header->p_prev = NULL;
		p_new_header->p_next = free_list[i];
		if (free_list[i]) {
			free_list[i]->p_prev = p_new_header;
		}
		free_list[i] = p_new_header;
	}
	p_header->free = 0;
	p_header->TID = osGetTID(); // does this fail on osTaskExit();
	p_header->free_list_index = i;
	p_header->magic_num = MAGIC_NUM;
	p_header->full_size = chunk_size;
	p_header->p_prev = NULL;
	p_header->p_next = NULL;

//	total_allocated_memory += p_header->full_size;
	return (void*)((int)p_header + (int)sizeof(Block_Header));
}

int k_mem_dealloc(void* p_mem) {
	TCB* p_task = &tcb_array[osGetTID()];

	p_task->SVC.FREE_p_mem = p_mem;
	__SVC(SVC_FREE);

	return p_task->SVC.FREE_status;
}

int k_mem_dealloc_SVC(void* p_mem) {
	if (
			!kernel_mem_init ||
			!p_mem ||
			((Block_Header*)((int)p_mem - (int)sizeof(Block_Header)))->free ||
			((Block_Header*)((int)p_mem - (int)sizeof(Block_Header)))->magic_num != MAGIC_NUM ||
			((Block_Header*)((int)p_mem - (int)sizeof(Block_Header)))->TID != osGetTID() ||
			((uint32_t)((int)p_mem - (int)sizeof(Block_Header))) >= heap_end
	) { return RTX_ERR; }

	Block_Header* p_header = (Block_Header*)((int)p_mem - (int)sizeof(Block_Header));
	Block_Header* p_buddy = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) ^ p_header->full_size) + (uint32_t)&_img_end);
	Block_Header* p_left = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) & ~p_header->full_size) + (uint32_t)&_img_end);
	Block_Header* p_right = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) | p_header->full_size) + (uint32_t)&_img_end);

	p_header->free = 1;
	p_header->TID = 0;

	total_allocated_memory -= p_header->full_size;

	while (
			p_header->free_list_index < NUM_LAYERS - 1 &&
			p_left->free &&
			p_right->free &&
			p_left->full_size == p_right->full_size &&
			(uint32_t)p_header < heap_end
	) {
		if (p_buddy == free_list[p_buddy->free_list_index]) {
			free_list[p_buddy->free_list_index] = p_buddy->p_next;
		}
		if (p_buddy->p_prev) {
			p_buddy->p_prev->p_next = p_buddy->p_next;
		}
		if (p_buddy->p_next) {
			p_buddy->p_next->p_prev = p_buddy->p_prev;
		}

		p_right->magic_num = 0;

		p_header = p_left;
		p_header->free_list_index++;
		p_header->full_size <<= 1;

		p_buddy = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) ^ p_header->full_size) + (uint32_t)&_img_end);
		p_left = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) & ~p_header->full_size) + (uint32_t)&_img_end);
		p_right = (Block_Header*)((((uint32_t)p_header - (uint32_t)&_img_end) | p_header->full_size) + (uint32_t)&_img_end);
	}

	int i = p_header->free_list_index;
	p_header->p_prev = NULL;
	p_header->p_next = free_list[i];
	if (free_list[i]) { free_list[i]->p_prev = p_header; }
	free_list[i] = p_header;

	return RTX_OK;
}

int k_mem_count_extfrag(size_t size){
	if (!kernel_mem_init) { return 0; }

	int count = 0;

	for (int i = 0; i < NUM_LAYERS; i++) {
		Block_Header* p_header = free_list[i];
		while (p_header) {
			if ((size_t)((int)p_header->full_size - (int)sizeof(Block_Header)) >= size) {
				return count;
			}
			++count;
			p_header = p_header->p_next;
		}
	}

	return count;
}














