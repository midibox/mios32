/*
	FreeRTOS Emu
*/


#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <JUCEqueue.h>


xSemaphoreHandle xSemaphoreCreateMutex(void) {
	return JUCESemaphoreCreateMutex();
}

signed portBASE_TYPE xSemaphoreTake(xSemaphoreHandle Semaphore, portTickType BlockTime) {
	return JUCESemaphoreTake(Semaphore);
}

signed portBASE_TYPE xSemaphoreGive(xSemaphoreHandle Semaphore) {
	return JUCESemaphoreGive(Semaphore);
}
