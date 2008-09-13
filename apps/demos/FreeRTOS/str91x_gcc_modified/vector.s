
	.global	Reset_Vector


	.equ	VectorAddress,		0xFFFFF030	/* VIC Vector address register address. */
	.equ	VectorAddressDaisy,	0xFC000030	/* Daisy VIC Vector address register */

	.equ    Mode_USR,       0x10
	.equ    Mode_FIQ,       0x11
	.equ    Mode_IRQ,       0x12
	.equ    Mode_SVC,       0x13
	.equ    Mode_ABT,       0x17
	.equ    Mode_UND,       0x1B
	.equ    Mode_SYS,       0x1F			/* available on ARM Arch 4 and later */

	.equ    I_Bit,          0x80			/* when I bit is set, IRQ is disabled */
	.equ    F_Bit,          0x40			/* when F bit is set, FIQ is disabled */

	.text
	.arm
	.section .vectors, "ax"


/* Exception vectors */

Reset_Vector:	ldr     pc, Reset_Addr
					ldr     pc, Undefined_Addr
					ldr     pc, SWI_Addr
					ldr     pc, Prefetch_Addr
					ldr     pc, Abort_Addr
					nop							/* Reserved vector */
					ldr     pc, IRQ_Addr
					ldr     pc, FIQ_Addr

/* Exception handlers address table */

Reset_Addr:			.long	_start
Undefined_Addr:	.long	UndefinedHandler
SWI_Addr:			.long	SWIHandler
Prefetch_Addr:		.long	PrefetchHandler
Abort_Addr:			.long	AbortHandler
						.long	0				/* reserved */
IRQ_Addr:			.long	IRQHandler
FIQ_Addr:			.long	FIQHandler

/* Exception Handlers */

/*******************************************************************************
* Macro Name     : SaveContext
* Description    : This macro used to save the context before entering
                   an exception handler.
* Input          : The range of registers to store.
* Output         : none
*******************************************************************************/

.macro	SaveContext reg1 reg2
		stmfd		sp!,{\reg1-\reg2,lr}	/* Save The workspace plus the current return */
										/* address lr_ mode into the stack */
		mrs		r1, spsr				/* Save the spsr_mode into r1 */
		stmfd		sp!, {r1}				/* Save spsr */
.endm

/*******************************************************************************
* Macro Name     : RestoreContext
* Description    : This macro used to restore the context to return from
                   an exception handler and continue the program execution.
* Input          : The range of registers to restore.
* Output         : none
*******************************************************************************/

.macro	RestoreContext reg1 reg2
		ldmfd		sp!, {r1}				/* Restore the saved spsr_mode into r1 */
		msr		spsr_cxsf, r1			/* Restore spsr_mode */
		ldmfd		sp!, {\reg1-\reg2,pc}^	/* Return to the instruction following */
										/* the exception interrupt */
.endm



.extern pxCurrentTCB
.extern ulCriticalNesting

.macro portSAVE_CONTEXT MACRO

	/* Push r0 as we are going to use the register. */ 					
	stmdb		sp!, {r0}

	/* Set R0 to point to the task stack pointer. */ 					
	stmdb		sp, {sp}^
	nop
	sub		sp, sp, #4
	ldmia		sp!, {r0}

	/* Push the return address onto the stack. */ 						
	stmdb		r0!, {lr}

	/* Now we have saved lr we can use it instead of r0. */ 				
	mov		lr, r0

	/* Pop r0 so we can save it onto the system mode stack. */ 			
	ldmia		sp!, {r0}

	/* Push all the system mode registers onto the task stack. */ 		
	stmdb		lr, {r0-lr}^
	nop
	sub		lr, lr, #60

	/* Push the spsr onto the task stack. */ 							
	mrs		r0, spsr
	stmdb		lr!, {r0}

	ldr		r0, =ulCriticalNesting 
	ldr		r0, [r0]
	stmdb		lr!, {r0}

	/* Store the new top of stack for the task. */ 						
	ldr		r1, =pxCurrentTCB
	ldr		r0, [r1]
	str		lr, [r0]

.endm


.macro portRESTORE_CONTEXT MACRO

	/* Set the lr to the task stack. */ 									
	ldr		r1, =pxCurrentTCB
	ldr		r0, [r1]
	ldr		lr, [r0]

	/* The critical nesting depth is the first item on the stack. */ 	
	/* Load it into the ulCriticalNesting variable. */ 					
	ldr		r0, =ulCriticalNesting
	ldmfd		lr!, {r1}
	str		r1, [r0]

	/* Get the SPSR from the stack. */ 									
	ldmfd		lr!, {r0}
	msr		spsr_cxsf, r0

	/* Restore all system mode registers for the task. */ 				
	ldmfd		lr, {r0-r14}^
	nop

	/* Restore the return address. */ 									
	ldr		lr, [lr, #+60]

	/* And return - correcting the offset in the LR to obtain the */ 	
	/* correct address. */ 												
	subs		pc, lr, #4

.endm




/*******************************************************************************
* Function Name  : IRQHandler
* Description    : This function called when IRQ exception is entered.
* Input          : none
* Output         : none
*******************************************************************************/

IRQHandler:

		portSAVE_CONTEXT					/* Save the context of the current task. */

		ldr	r0, = VectorAddress
		ldr	r0, [r0]						/* Read the routine address */
		ldr	r1, = VectorAddressDaisy
		ldr	r1, [r1]
		mov	lr, pc
		bx		r0
		ldr	r0, = VectorAddress			/* Write to the VectorAddress to clear the */
		str	r0, [r0]							/* respective interrupt in the internal interrupt */
		ldr	r1, = VectorAddressDaisy	/* Write to the VectorAddressDaisy to clear the */
		str	r1,[r1]							/* respective interrupt in the internal interrupt*/
	
		portRESTORE_CONTEXT					/* Restore the context of the selected task. */


/*******************************************************************************
* Function Name  : SWIHandler
* Description    : This function called when SWI instruction executed.
* Input          : none
* Output         : none
*******************************************************************************/

SWIHandler:
		add	lr, lr, #4
		portSAVE_CONTEXT        
		ldr 	r0, =vTaskSwitchContext
		mov 	lr, pc
		bx 	r0
		portRESTORE_CONTEXT 

/*******************************************************************************
* Function Name  : UndefinedHandler
* Description    : This function called when undefined instruction
                   exception is entered.
* Input          : none
* Output         : none
*******************************************************************************/

UndefinedHandler:
		SaveContext r0, r12
		bl		Undefined_Handler
		RestoreContext r0, r12

/*******************************************************************************
* Function Name  : PrefetchAbortHandler
* Description    : This function called when Prefetch Abort
                   exception is entered.
* Input          : none
* Output         : none
*******************************************************************************/

PrefetchHandler:
		sub		lr, lr, #4			/* Update the link register. */
		SaveContext r0, r12
		bl		Prefetch_Handler
		RestoreContext r0, r12

/*******************************************************************************
* Function Name  : DataAbortHandler
* Description    : This function is called when Data Abort
                   exception is entered.
* Input          : none
* Output         : none
*******************************************************************************/

AbortHandler:
		sub		lr, lr, #8			/* Update the link register. */
		SaveContext r0, r12
		bl		Abort_Handler
		RestoreContext r0, r12

/*******************************************************************************
* Function Name  : FIQHandler
* Description    : This function is called when FIQ
                   exception is entered.
* Input          : none
* Output         : none
*******************************************************************************/

FIQHandler:
		sub		lr, lr, #4			/* Update the link register. */
		SaveContext r0, r7
		bl		FIQ_Handler
		RestoreContext r0, r7

	.end
