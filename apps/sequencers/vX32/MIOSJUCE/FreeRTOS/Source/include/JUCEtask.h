/*
	FreeRTOS Emu
*/

#ifndef JUCETASK_H
#define JUCETASK_H

#ifdef __cplusplus
extern "C" {
#endif

extern void JUCETaskCreate(void (*pvTaskCode)(void * ), const signed char *pcName, 
					void *pvParameters, int uxPriority, int *pxCreatedTask);

extern long JUCETaskGetTickCount(void);

extern void JUCETaskDelay(long time);

extern void JUCETaskSuspend(int taskID);

extern void JUCETaskResume(int taskID);

extern void JUCETaskSuspendAll(void);

extern void JUCETaskResumeAll(void);

#ifdef __cplusplus
}
#endif


#endif /* JUCETASK_H */



