
.global _start

/* set these dependent on memory sizes */

	.equ	SRAM_96K,			0 		/* 0 = 64k, 1 = 96k ram */
	.equ 	FLASH_512K,			0		/* 0 = 256k, 1 = 512k flash */
	.equ	BUFFERED_MODE,		0		/* 0 = non-buffered, 1 = buffered */

	
	
/* Stack Sizes */
    .set  UND_STACK_SIZE, 0x00000020
    .set  ABT_STACK_SIZE, 0x00000020
    .set  FIQ_STACK_SIZE, 0x00000004
    .set  IRQ_STACK_SIZE, 0X00001000
    .set  SVC_STACK_SIZE, 0x00002000
	

/* some definitions */

	.equ	MODE_USR,		0x10
	.equ	MODE_FIQ,		0x11
	.equ	MODE_IRQ,		0x12
	.equ	MODE_SVC,		0x13
	.equ	MODE_ABT,		0x17
	.equ	MODE_UND,		0x1B
	.equ	MODE_SYS,		0x1F			/* available on ARM Arch 4 and later */

	.equ	I_BIT,			0x80			/* when I bit is set, IRQ is disabled */
	.equ	F_BIT,			0x40			/* when F bit is set, FIQ is disabled */


/* System configuration register 0 unbuffered */
	.equ	SCU_SCRO,		0x5C002034		

/* FMI registers unbuffered */
	.equ	FMI_BASE,		0x54000000		
	.equ	FMI_BBSR,		0x00		
	.equ	FMI_NBBSR,		0x04
	.equ	FMI_BBADR,		0x0C
	.equ	FMI_NBBADR,		0x10
	.equ	FMI_CR,			0x18


	.equ SRAM_START,		0x04000000


.if SRAM_96K
	.equ	SRAM_SIZE,		0x10 				/* 96k sram */
	.equ	SRAM_END,		0x04018000		/* at the top of 96 KB SRAM */
.else
	.equ	SRAM_SIZE,		0x08				/* 64k sram */
	.equ	SRAM_END,		0x04010000		/* at the top of 64 KB SRAM */
.endif

.if FLASH_512K
	.equ	FLASH_SIZE,		0x04		/* 512k flash */
.else
	.equ	FLASH_SIZE,		0x03		/* 256k flash */
.endif

	


_start:
		ldr     pc, =NextInst
NextInst:

		nop		/* Wait for OSC stabilization */
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop



.if BUFFERED_MODE
		mrc		p15, 0, r0, c1, c0, 0		/* Read CP15 register 1 into r0 */
		orr		r0, r0, #0x8					/* Enable Write Buffer on AHB */
		mcr		p15, 0, r0, c1, c0, 0 		/* Write CP15 register 1 */
.endif



/* Remap Flash Bank 0 at address 0x0 and Bank 1 at address 0x80000, */
/* when the bank 0 is the boot bank, then enable the Bank 1. */

		ldr		r0, = FMI_BASE
		ldr		r1, = FLASH_SIZE				/* configure 256k/512k Boot bank 0 */
		str		r1, [r0, #FMI_BBSR]

		ldr		r1, = 0x2						/* configure 32KB Non Boot bank 1 */
		str		r1, [r0, #FMI_NBBSR]

		ldr		r1, = (0x00000000 >> 2)		/* Boot Bank Base Address */
		str		r1, [r0, #FMI_BBADR]

		ldr		r1, = (0x00080000 >> 2)		/* Non Boot Bank Base Address */
		str		r1, [r0, #FMI_NBBADR]

		ldr		r1, = 0x18						/* Flash Banks 0 1 enabled */
		str		r1, [r0, #FMI_CR]


/* Enable RAM */

		ldr		r0, = SCU_SCRO
		ldr		r1, = 0x0187 | SRAM_SIZE
		str		r1, [r0]


/* Set bits 17-18 (Instruction/Data TCM order) of the */
/* Core Configuration Control Register */
 
		mov		r0, #0x60000
		mcr		p15, 0x1, r0, c15, c1, 0
		

/* setup stacks */

		ldr		r0, = SRAM_END
		msr		CPSR_c, #MODE_UND|I_BIT|F_BIT /* Undefined Instruction Mode */
		mov		sp, r0
		sub		r0, r0, #UND_STACK_SIZE
		msr		CPSR_c, #MODE_ABT|I_BIT|F_BIT /* Abort Mode */
		mov		sp, r0
		sub		r0, r0, #ABT_STACK_SIZE
		msr		CPSR_c, #MODE_FIQ|I_BIT|F_BIT /* FIQ Mode */
		mov		sp, r0
		sub		r0, r0, #FIQ_STACK_SIZE
		msr		CPSR_c, #MODE_IRQ|I_BIT|F_BIT /* IRQ Mode */
		mov		sp, r0
		sub		r0, r0, #IRQ_STACK_SIZE
		msr		CPSR_c, #MODE_SVC|I_BIT|F_BIT /* Supervisor Mode */
		mov		sp, r0
		sub		r0, r0, #SVC_STACK_SIZE
		msr		CPSR_c, #MODE_SYS|I_BIT|F_BIT /* System Mode */
		mov		sp, r0
    

/* switch to supervisor mode*/	
		msr		CPSR_c, #MODE_SVC|I_BIT|F_BIT
	
	
/* Clear uninitialized data section (bss) */
    	ldr		r4, =__bss_start__     			/* First address*/      
    	ldr		r5, =__bss_end__       			/* Last  address*/      
    	mov		r6, #0x0                                
    
loop_zero:                                            
		str		r6, [r4]                                
    	add		r4, r4, #0x4                            
    	cmp		r4, r5                                  
    	blt		loop_zero

/* copy initialised data */
    	ldr		r4, = _etext      			/* First address from*/      
    	ldr		r5, = __data_start  			/* First  address to*/       
    	ldr		r7, = _edata					/* end of copy */

    	cmp		r5, r7                                  
    	beq		skip_initialise
	
loop_initialise:                                            
		ldr		r6, [r4]
		str		r6, [r5]                                
    	add		r4, r4, #0x4                            
    	add		r5, r5, #0x4                            
    	cmp		r5, r7                                  
    	blt		loop_initialise    
    
skip_initialise:



/* --- Now enter the C code */
		B       main   			/* Note : use B not BL, because an application will */
                         		/* never return this way */



.end
