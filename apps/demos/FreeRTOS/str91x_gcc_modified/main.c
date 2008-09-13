/*
	FreeRTOS.org V4.7.0 - Copyright (C) 2003-2007 Richard Barry.

	This file is part of the FreeRTOS distribution.

	FreeRTOS is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS, without being obliged to provide
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

/*
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/

/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 *
 * A few tasks are created that are not part of the standard demo.  These are
 * the 'LCD' task, the 'LCD Message' task, a WEB server task and the 'Check'
 * task.
 *
 * The LCD task is the only task that accesses the LCD directly, so mutual
 * exclusion is ensured.  Any task wishing to display text sends the LCD task
 * a message containing a pointer to the string that should be displayed.
 * The LCD task itself just blocks on a queue waiting for such a message to
 * arrive - processing each in turn.
 *
 * The LCD Message task does nothing other than periodically send messages to
 * the LCD task.  The messages originating from the LCD Message task are
 * displayed on the top row of the LCD.
 *
 * The Check task only executes every three seconds but has the highest
 * priority so is guaranteed to get processor time.  Its main function is to
 * check that all the other tasks are still operational. Most tasks maintain
 * a unique count that is incremented each time the task successfully completes
 * a cycle of its function.  Should any error occur within such a task the
 * count is permanently halted.  The check task sets a bit in an error status
 * flag should it find any counter variable at a value that indicates an
 * error has occurred.  The error flag value is converted to a string and sent
 * to the LCD task for display on the bottom row on the LCD.
 */

/* Standard includes. */
#include <stdio.h>

/* Library includes. */
#include "91x_lib.h"

#include "hw_config.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"



/* Task priorities. */
#define mainQUEUE_POLL_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainFLASH_PRIORITY                  ( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 3 )
#define mainGEN_QUEUE_TASK_PRIORITY			( tskIDLE_PRIORITY ) 


static void prvSetupHardware( void );
int main( void );


static void vLEDFlashTask( void *pvParameters );



/*
 * Starts all the other tasks, then starts the scheduler.
 */
int main( void )
{
	
	/* Setup any hardware that has not already been configured by the low
	level init routines. */
	prvSetupHardware();

	xTaskCreate( vLEDFlashTask, ( signed portCHAR * ) "LED", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY - 1, NULL );
	

	vTaskStartScheduler();

	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	
	FMI->BBSR   = 4;
	FMI->NBBSR  = 2;
	FMI->BBADR  = (0 >> 2);
	FMI->NBBADR = (0x80000 >> 2);
	FMI->CR |= 0x18; /* Enable bank 1 */
	FMI->CR &= FMI_WRITE_WAIT_STATE_0;
	
	*(vu16 *)FMI_BANK_1 = 0x60;
	
	/* need 3 read wait states if operating @ 96Mhz */
	*(vu16 *)(FMI_BANK_1 | FMI_READ_WAIT_STATE_3 | FMI_PWD_ENABLE | FMI_LVD_ENABLE | FMI_FREQ_HIGH) = 0x03;	
	

	InitInterruptVecotrs();

	InitGPIOPorts();
	GPIO3->DR[PORT_BIT0] = 0; /* turn off lcd backlight */
	
	/* turn on power bits */	
	//	GPIO0->DR[( PORT_BIT7 | PORT_BIT6) ] = 0xc0;
	//	HW_SDCard_CS_High;

	//	InitEMIBus();
	//	GoFast();	
	
	/*RTC_Init(); */
	
}

static void vLEDFlashTask( void *pvParameters )
{
	portTickType xLastWakeTime;
	
	xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, 960 );
		GPIO9->DR[PORT_BIT0] = 1;	

		vTaskDelayUntil( &xLastWakeTime, 40 );
		GPIO9->DR[PORT_BIT0] = 0;	
	}
}


