/*
	FreeRTOS.org V4.7.0 - Copyright (C) 2003-2007 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license
	and contact details.  Please ensure to read the configuration and relevant
	port sections of the online documentation.

	Also see http://www.SafeRTOS.com a version that has been certified for use
	in safety critical systems, plus commercial licensing, development and
	support options.
	***************************************************************************
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ST STR75x ARM7
 * port.
 *----------------------------------------------------------*/

/* Library includes. */
#include "91x_lib.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Constants required to setup the initial stack. */
#define portINITIAL_SPSR				( ( portSTACK_TYPE ) 0x1f ) /* System mode, ARM mode, interrupts enabled. */
#define portTHUMB_MODE_BIT				( ( portSTACK_TYPE ) 0x20 )
#define portINSTRUCTION_SIZE			( ( portSTACK_TYPE ) 4 )

/* Constants required to handle critical sections. */
#define portNO_CRITICAL_NESTING 		( ( unsigned portLONG ) 0 )

static u16 s_nPulseLength;
static void SysTick_IRQHandler( void );


/*-----------------------------------------------------------*/

/* Setup the TB to generate the tick interrupts. */
static void prvSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See header file for description.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
portSTACK_TYPE *pxOriginalTOS;

	pxOriginalTOS = pxTopOfStack;

	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* First on the stack is the return address - which in this case is the
	start of the task.  The offset is added to make the return address appear
	as it would within an IRQ ISR. */
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode + portINSTRUCTION_SIZE;		
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xaaaaaaaa;	/* R14 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) pxOriginalTOS; /* Stack used when task starts goes in R13. */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x12121212;	/* R12 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x11111111;	/* R11 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x10101010;	/* R10 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x09090909;	/* R9 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x08080808;	/* R8 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x07070707;	/* R7 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x06060606;	/* R6 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x05050505;	/* R5 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x04040404;	/* R4 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x03030303;	/* R3 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x02020202;	/* R2 */
	pxTopOfStack--;	
	*pxTopOfStack = ( portSTACK_TYPE ) 0x01010101;	/* R1 */
	pxTopOfStack--;	

	/* When the task starts is will expect to find the function parameter in
	R0. */
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters; /* R0 */
	pxTopOfStack--;

	/* The status register is set for system mode, with interrupts enabled. */
	*pxTopOfStack = ( portSTACK_TYPE ) portINITIAL_SPSR;

	#ifdef THUMB_INTERWORK
	{
		/* We want the task to start in thumb mode. */
		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}
	#endif

	pxTopOfStack--;

	/* Interrupt flags cannot always be stored on the stack and will
	instead be stored in a variable, which is then saved as part of the
	tasks context. */
	*pxTopOfStack = portNO_CRITICAL_NESTING;

	return pxTopOfStack;	
}
/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
extern void vPortISRStartFirstTask( void );

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	prvSetupTimerInterrupt();

	/* Start the first task. */
	vPortISRStartFirstTask();	

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the ARM port will require this function as there
	is nothing to return to.  */
}
/*-----------------------------------------------------------*/



static void prvFindFactors(u32 n, u8 *a, u16 *b)
{
	/* This function is copied from the ST STR7 library and is
	copyright STMicroelectronics.  Reproduced with permission. */

	u16 b0;
	u8 a0;
	long err, err_min=n;


	*a = a0 = ((n-1)/256) + 1;
	*b = b0 = n / *a;

	for (; *a <= 256; (*a)++)
	{
		*b = n / *a;
		err = (long)*a * (long)*b - (long)n;
		if (abs(err) > (*a / 2))
		{
			(*b)++;
			err = (long)*a * (long)*b - (long)n;
		}
		if (abs(err) < abs(err_min))
		{
			err_min = err;
			a0 = *a;
			b0 = *b;
			if (err == 0) break;
		}
	}

	*a = a0;
	*b = b0;
}


static void prvSetupTimerInterrupt( void )
{
	unsigned portCHAR a;
	unsigned portSHORT b;
	unsigned portLONG n;
	

	/* enable perhiperal */
	if((configUSE_TIMER == TIM0) || (configUSE_TIMER == TIM1) ) {
		/* enable timer 0/1 */
		SCU->PCGR1 |= 1;
		SCU->PRR1 |= 1;	
	} else {
		/* enable timer 2/3 */
		SCU->PCGR1 |= 2;
		SCU->PRR1 |= 2;
	}
		
	/* reset timer */
	configUSE_TIMER->CR1 = 0;			/* disable timer */
	configUSE_TIMER->CR2 = 0;	
	configUSE_TIMER->SR  = 0; 			/* clear any interrupt events */
	
	/* setup VIC */
    
    if (configUSE_TIMER == TIM0 ) {
        a = TIM0_ITLine;
    } else if (configUSE_TIMER == TIM1 ) {
        a = TIM1_ITLine;
    } else if (configUSE_TIMER == TIM2 ) {
        a = TIM2_ITLine;
    } else if (configUSE_TIMER == TIM3 ) {
        a = TIM3_ITLine;    	
    }
       
    
    /* enable VIC */
	SCU->PCGR0 |= __VIC;
	SCU->PRR0 |= __VIC;
    
    VIC0->VAiR[configSysTickIRQPriority] = (u32)SysTick_IRQHandler; 	/* set interrupt handler */
    VIC0->VCiR[configSysTickIRQPriority] = 0x20 | a;

        
    VIC0->INTSR &= ~(1 << a);
    VIC0->INTER |= (1 << a);	/* enable TIM interrupt */
	
	
	
	n = configCPU_PERIPH_HZ / configTICK_RATE_HZ;
	prvFindFactors( n, &a, &b );
	s_nPulseLength = b-1;
	

	configUSE_TIMER->CR2 |=  (a-1);			/* set clock divider */
	configUSE_TIMER->CR2 |= 0x4000; 		/* enable OC1 interrupt */
    configUSE_TIMER->OC1R = s_nPulseLength;	/* set period */
	configUSE_TIMER->CNTR = 0x1234; 		/* reset count */
	configUSE_TIMER->CR1 |= 0x8000;			/* start timer */

}
/*-----------------------------------------------------------*/


static void SysTick_IRQHandler( void )
{
	if(configUSE_TIMER->SR & TIM_FLAG_OC1) {
		/* Reset the timer counter to avoid overflow. */
		configUSE_TIMER->OC1R += s_nPulseLength;
		
		/* Increment the tick counter. */
		vTaskIncrementTick();
		
		#if configUSE_PREEMPTION == 1
		{
			/* The new tick value might unblock a task.  Ensure the highest task that
			is ready to execute is the task that will execute when the tick ISR
			exits. */
			vTaskSwitchContext();
		}
		#endif
		
		configUSE_TIMER->SR &= ~TIM_FLAG_OC1;
	}
	
}
