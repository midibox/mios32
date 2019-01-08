/*
	FreeRTOS Emu
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "StackMacros.h"

#include <JUCEtask.h>


signed portBASE_TYPE xTaskCreate( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, unsigned portSHORT usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask ) {
	// discarded for JUCE emulation: unsigned portSHORT usStackDepth
	JUCETaskCreate(pvTaskCode, pcName, pvParameters, uxPriority, pxCreatedTask);
	return 0;
}

portTickType xTaskGetTickCount(void) {
	return JUCETaskGetTickCount();
}

void vTaskDelay( portTickType xTicksToDelay ) {
	JUCETaskDelay(xTicksToDelay);
}

void vTaskDelayUntil( portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement ) {
	JUCETaskDelay(xTimeIncrement);	
}

void vTaskSuspend( xTaskHandle pxTaskToSuspend ) {
	JUCETaskSuspend(pxTaskToSuspend);	
}

void vTaskResume( xTaskHandle pxTaskToResume ) {
	JUCETaskResume(pxTaskToResume);	
}

void vTaskSuspendAll(void) {
	JUCETaskSuspendAll();
}

void xTaskResumeAll(void) {
	JUCETaskResumeAll();
}


