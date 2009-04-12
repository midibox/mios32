/*
	FreeRTOS Emu
*/


/*
 * Implementation of pvPortMalloc() and vPortFree() that relies on the
 * compilers own malloc() and free() implementations.
 *
 * This file can only be used if the linker is configured to to generate
 * a heap memory area.
 *
 * See heap_2.c and heap_1.c for alternative implementations, and the memory
 * management pages of http://www.FreeRTOS.org for more information.
 */

#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>

/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
void *pvReturn;

	//vTaskSuspendAll();
	{
		pvReturn = malloc( xWantedSize );
	}
	//xTaskResumeAll();

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
	if( pv )
	{
		//vTaskSuspendAll();
		{
			free( pv );
		}
		//xTaskResumeAll();
	}
}



