/*
	FreeRTOS Emu
*/

#ifndef JUCEQUEUE_H
#define JUCEQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

extern xSemaphoreHandle JUCESemaphoreCreateMutex(void);

extern int JUCESemaphoreTake(xSemaphoreHandle Semaphore);

extern int JUCESemaphoreGive(xSemaphoreHandle Semaphore);

#ifdef __cplusplus
}
#endif


#endif /* JUCEQUEUE_H */



