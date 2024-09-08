  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.extern osScheduler
.global PendSV_Handler

.thumb_func
PendSV_Handler:
	MRS     R0, PSP
	STMDB	R0!, {R4-R11}
	MSR     PSP, R0

    BL 		osScheduler

	MRS     R0, PSP
	LDMIA 	R0!, {R4-R11}
	MSR		PSP, R0

	MOV LR, #0xFFFFFFFD			// put magic number into LR
	BX LR						// a "branch" to this magic number will


