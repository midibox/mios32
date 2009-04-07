#include <mios32.h>

// todo
// if this is to be complete, it needs to have an emulation 
// for all the MIOS32 and FREERTOS functions....
// 
// Let's just start with what the vX needs
// and we can move on/get contributions later..
// so here's what needs to be done first:

// mios32


// not to forget trunk/programming_models/traditional/moin.c .......


/*
MIOS32_BOARD_LED_Init
MIOS32_BOARD_LED_Set
*/


#define dummyreturn 0 // just something to keep the compiler quiet for now

s32 MIOS32_BOARD_LED_Init(u32 leds) {
	return dummyreturn;
}

s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value) {
	return dummyreturn;
}


/*
MIOS32_DELAY_Init
*/

s32 MIOS32_DELAY_Init(u32 mode) {
	return dummyreturn;
}


/*
MIOS32_IRQ_Enable
MIOS32_IRQ_Disable
*/

s32 MIOS32_IRQ_Disable(void) {
	return dummyreturn;
}

s32 MIOS32_IRQ_Enable(void) {
	return dummyreturn;
}


/*
MIOS32_LCD_Clear
MIOS32_LCD_BColourSet
MIOS32_LCD_FColourSet
MIOS32_LCD_CursorSet
MIOS32_LCD_PrintString
*/

s32 MIOS32_LCD_Clear(void) {
	return dummyreturn;
}

s32 MIOS32_LCD_BColourSet(u8 r, u8 g, u8 b) {
	return dummyreturn;
}

s32 MIOS32_LCD_FColourSet(u8 r, u8 g, u8 b) {
	return dummyreturn;
}

s32 MIOS32_LCD_CursorSet(u16 column, u16 line) {
	return dummyreturn;
}

s32 MIOS32_LCD_PrintString(char *str) {
	return dummyreturn;
}


/*
MIOS32_MIDI_DirectRxCallback_Init
MIOS32_MIDI_SendCC
MIOS32_MIDI_SendClock
MIOS32_MIDI_SendStart
MIOS32_MIDI_SendContinue
MIOS32_MIDI_SendStop
MIOS32_MIDI_SendPackage
*/

s32 MIOS32_MIDI_DirectRxCallback_Init(void *callback_rx) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port) {
	return dummyreturn;
}

s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package) {
	return dummyreturn;
}


/*
MIOS32_TIMER_ReInit
MIOS32_TIMER_Init
*/

s32 MIOS32_TIMER_Init(u8 timer, u32 period, void *_irq_handler, u8 irq_priority) {
	return dummyreturn;
}

s32 MIOS32_TIMER_ReInit(u8 timer, u32 period) {
	return dummyreturn;
}





// see also ../FreeRTOS/Source/queue.c
/*
xSemaphoreHandle xSemaphoreCreateMutex(void);
signed portBASE_TYPE xSemaphoreTake(xSemaphoreHandle Semaphore, portTickType BlockTime);
signed portBASE_TYPE xSemaphoreGive(xSemaphoreHandle Semaphore);
*/


// see also ../FreeRTOS/Source/task.c
/*
signed portBASE_TYPE xTaskCreate( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, unsigned portSHORT usStackDepth, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask );
portTickType xTaskGetTickCount(void);
void vTaskDelay( portTickType xTicksToDelay );
void vTaskDelayUntil( portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement );
void vTaskResume( xTaskHandle pxTaskToResume );
void vTaskSuspendAll(void);
void xTaskResumeAll(void);
*/
