# Real_Time_Executive
A RTX that can perform pre-emptive multitasking, allocation &amp; deallocation using buddy systems

## Objective
RTX is able to:
1. Dynamically allocate task stacks based on a desired size (using our own allocation functions)
2. Assign a priority to each task depending on its deadline
3. Pre-emptively switch a task if it has exhausted its time slice (exceeded its deadline)
4. Allow running tasks to change the priorities (deadlines) of others, potentially triggering a context
switch
5. Allow tasks to sleep for a set period of time, during which they are not scheduled

The dynamic memory management system implemented based on the Buddy scheme has:
1. A memory-initialization function, which sets up your RTX and the microcontroller’s memory
2. Allocation and deallocation functions, modifying your Deliverable 1 code if needed to implement the concept of memory ownership
3. A utility function to analyze the efficiency of the allocation algorithm and its implementation

## Functions Implemented
### Task Sleep Function
**Prototype**: void osSleep(int timeInMs)

**Description**: This function immediately halts the execution of one task, saves it context, runs the scheduler, and loads the context of the next task to run. In addition, this function removes this task from scheduling until the provided sleep time has expired. Upon awakening, a task’s deadline is reset to its initial value. If no tasks are available to run (that is, all tasks are sleeping), the expected behaviour is for the NULL task to run until a task emerges from sleep.

This function operates the same as osYield except the task cannot be scheduled until the sleep time is up.

**Return value**: None 

----------------------------------
### Periodic Yield Function
**Prototype**: void osPeriodYield()

**Description**: A periodic task will call this function instead of osYield when it has completed its current instance. The task is not ready to execute until its current time period elapses, so it should not be scheduled immediately even if the CPU is available. In practice, this function can have the same behavior as osSleep, except that the sleep time is calculated automatically by the Kernel to facilitate the task’s periodicity.

Return value: None

----------------------------------
### Deadline Changing Function
**Prototype**: int osSetDeadline(int deadline, task_t TID)

**Description**: This function sets the deadline of the task with Id TID to the deadline stored in the deadline variable. Any task can set the deadline of any other task (but not itself). This function must block – that is, when it is called, the OS should not respond to any timer interrupts until it is completed. It is possible for this function to cause a pre-emptive context switch in the following scenario: presume task A is currently running and has a deadline of 5ms. task B is not running and has a deadline of 10ms. If task A calls this function to set task B’s deadline to 3ms, task B now has an earlier deadline and task A should be pre-empted.

**Return value**: this function should return RTX_OK if the deadline provided is strictly positive and a task with the given TID exists and is ready to run (in READY state) . Otherwise it returns RTX_ERR.

----------------------------------
### Task Creation with Deadline Function
**Prototype**: int osCreateDeadlineTask(int deadline, TCB* task)

**Description**: Create a new task and register it with the RTX if possible. This task has the deadline given in the deadline variable and stack size given by the task->stack_size variable, which is assumed to be in bytes. Its operation is the same as osCreateTask except for the custom deadline.
The caller of this function never blocks, but it could be pre-empted. Specifically, in the case where the calling task has a longer deadline than the newly created task, a pre-emptive context switch must occur.

This function must dynamically allocate a stack of the given size using the memory management functions from deliverable 2. The owner of the new stack should be the newly created task.
Return value: this function returns RTX_OK on success and RTX_ERR on failure. Failure happens when a new task cannot be created because of invalid inputs or if the RTX cannot allocate a new task. Invalid inputs include the same cases as discussed in osCreateTask for deliverable 1, as well as the deadline being zero or strictly negative or the stack size being less than STACK_SIZE or too large given the amount of free memory available.

----------------------------------
### Memory Initialization Function
**Prototype**:int k_mem_init()

**Description**: Initializes the RTX’s memory manager. This should include locating the heap and writing the initial metadata required to start searching for free regions. As the manager allocates and deallocates memory(seek_mem_alloc andk_mem_dealloc),the memory will be partitionedintofreeand allocated regions. Your metadata data structures will be used to keep track of these regions.

**Return value**: Returns RTX_OK on success and RTX_ERR on failure, which happens if this function is called more than once or if it is called before the kernel itself is initialized.

----------------------------------
### Memory Allocation Function
**Prototype**:void* k_mem_alloc(size_t size)

**Description**: Allocates size bytes according to the Buddy algorithm, and returns a pointer to the start of the usable memory in the block. The k_mem_init function must be called before this function is called, otherwise this function must return NULL. The size argument is the number of bytes requested. If metadata is stored within the block,it must be allocated in addition to size bytes.If size is 0,this function returns NULL. The allocated memory is not initialized. Memory requests may be of any size.

By default this function assigns the memory region requested to the currently running task, or to the kernel if no tasks are running. This ownership information must be stored in the metadata, typically by storing the task ID of the task that allocated the memory, or some obvious value if the kernel allocated the memory.

**Return value**: Returns a pointer to allocated memory or NULL if the request fails. Failure happens if the RTX cannot allocate the requested memory, either for a reason stated above or because there are no blocks of suitable size available.

----------------------------------
### Memory Deallocation Function
**Prototype**:int k_mem_dealloc(void* ptr)

**Description**: this function frees the memory pointed to by ptr as long as the currently running task is the owner of that block and as long as the memory pointed to by ptr is in fact allocated (see below). Otherwise, or if this memory is already free, this function should return RTX_ERR. If ptr is NULL, no action is performed. If the newly freed memory region is eligible for coalescence according to the Buddy System, it must be coalesced accordingly and the combined region is re-integrated into the free memory under management. This function does not clear the content of the newly freed region.

During testing, it is possible we will attempt to free a random/invalid pointer, therefore your program should first check the validity of the pointer. For an example countermeasure, you can store a secret value in the metadata prepended to each block, and check against it for each deallocation request. We will not attempt to corrupt such metadata. Please note a pointer is only valid for deallocation if it points to the start of the user buffer (i.e. was previously returned by k_mem_alloc).

**Return value**: returns RTX_OK on success and RTX_ERR on failure. Failure happens when the RTX cannot successfully free the memory region for some reason (some of which are explained above).

----------------------------------
### Utility Function
**Prototype**:int k_mem_count_extfrag(size_t size);

**Description**: This function counts the number of free memory regions that are strictly less than size bytes. If your metadata is inside the memory blocks, the space your memory management data structures occupy inside of each free region is considered to be free in this context, but not each allocated region. 
