/*
	FreeRTOS Emu
*/

#ifndef PROJDEFS_H
#define PROJDEFS_H

/* Defines the prototype to which task functions must conform. */
typedef void (*pdTASK_CODE)( void * );

#define pdTRUE		( 1 )
#define pdFALSE		( 0 )

#define pdPASS									( 1 )
#define pdFAIL									( 0 )
#define errQUEUE_EMPTY							( 0 )
#define errQUEUE_FULL							( 0 )

/* Error definitions. */
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY	( -1 )
#define errNO_TASK_TO_RUN						( -2 )
#define errQUEUE_BLOCKED						( -4 )
#define errQUEUE_YIELD							( -5 )

#endif /* PROJDEFS_H */



