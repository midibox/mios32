/*
	FreeRTOS Emu
*/


#ifndef INC_FREERTOS_H
	#error "#include FreeRTOS.h" must appear in source files before "#include task.h"
#endif



#ifndef TASK_H
#define TASK_H

#include "portable.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define tskKERNEL_VERSION_NUMBER "JUCE"

#define tskIDLE_PRIORITY			( ( unsigned portBASE_TYPE ) 0 )
#define portTICK_RATE_MS 1

typedef int xTaskHandle;


signed portBASE_TYPE xTaskCreate( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, unsigned portSHORT usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask );

portTickType xTaskGetTickCount(void);

void vTaskDelay( portTickType xTicksToDelay );

void vTaskDelayUntil( portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement );

void vTaskSuspend( xTaskHandle pxTaskToSuspend );

void vTaskResume( xTaskHandle pxTaskToResume );

void vTaskSuspendAll(void);

void xTaskResumeAll(void);


#ifdef __cplusplus
}
#endif

#endif /* TASK_H */



