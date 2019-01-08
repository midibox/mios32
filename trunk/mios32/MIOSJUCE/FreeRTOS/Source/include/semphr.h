/*
	FreeRTOS Emu
*/

#ifndef INC_FREERTOS_H
	#error "#include FreeRTOS.h" must appear in source files before "#include semphr.h"
#endif


typedef int xSemaphoreHandle;

xSemaphoreHandle xSemaphoreCreateMutex(void);
signed portBASE_TYPE xSemaphoreTake(xSemaphoreHandle Semaphore, portTickType BlockTime);
signed portBASE_TYPE xSemaphoreGive(xSemaphoreHandle Semaphore);


#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "queue.h"

#endif /* SEMAPHORE_H */


