  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global SVC_Handler
.extern SVC_Handler_Main

.thumb_func
SVC_Handler:
	TST LR, #4 // check LR to know which stack is used
	ITE EQ // 2 next instructions are conditional
	MRSEQ R0, MSP // save MSP if bit 2 is 0
	MRSNE R0, PSP // save PSP if bit 2 is 1

	B SVC_Handler_Main
