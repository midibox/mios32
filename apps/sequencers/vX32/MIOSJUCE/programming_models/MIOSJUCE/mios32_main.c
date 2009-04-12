/*
	MIOS32 Emu
*/

#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>

#include <mios32.h>
#include "mios32_main.h"

#include <app.h>

// needed because we use the #define'd number of timers
// to find the last timer available, to emulate the 1ms tick hook
#include <JUCEtimer.h>


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_HOOKS		( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters);




// not to forget trunk/programming_models/traditional/moin.c .......
// here's a fake version of the main() from real MIOS32

void MIOS32_Main(void) {
	
// lots of todos here!


  // initialize hardware and MIOS32 modules
  
 /*
#ifndef MIOS32_DONT_USE_SYS
  MIOS32_SYS_Init(0);
#endif
#ifndef MIOS32_DONT_USE_DELAY
  MIOS32_DELAY_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SPI
  MIOS32_SPI_Init(0);
#endif
#ifndef MIOS32_DONT_USE_SRIO
  MIOS32_SRIO_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DIN_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_DOUT) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_DOUT_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_ENC) && !defined(MIOS32_DONT_USE_SRIO)
  MIOS32_ENC_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_MF)
  MIOS32_MF_Init(0);
#endif
#if !defined(MIOS32_DONT_USE_AIN)
  MIOS32_AIN_Init(0);
#endif
#ifndef MIOS32_DONT_USE_IIC_BS
  MIOS32_IIC_BS_Init(0);
#endif
#ifndef MIOS32_DONT_USE_USB
  MIOS32_USB_Init(0);
#endif
*/
#ifndef MIOS32_DONT_USE_MIDI
  MIOS32_MIDI_Init(0);
#endif
/*
#ifndef MIOS32_DONT_USE_OSC
  MIOS32_OSC_Init(0);
#endif
#ifndef MIOS32_DONT_USE_COM
  MIOS32_COM_Init(0);
#endif
#ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_Init(0);
#endif
#ifdef MIOS32_USE_I2S
  MIOS32_I2S_Init(0);
#endif
*/
  // initialize application
  APP_Init();
/*
  // print boot message
#ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_PrintBootMessage();
#endif

  // wait for 2 seconds
#ifndef MIOS32_DONT_USE_DELAY
  int delay = 0;
  for(delay=0; delay<2000; ++delay)
    MIOS32_DELAY_Wait_uS(1000);
#endif
*/
  // start the task which calls the application hooks
  xTaskCreate(TASK_Hooks, (signed portCHAR *)"Hooks", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_HOOKS, NULL);


#if ( configUSE_IDLE_HOOK == 1 )
{
    extern void vApplicationIdleHook(void *pvParameters);

    /* Call the user defined function from within the idle task.  This
    allows the application designer to add background functionality
    without the overhead of a separate task.
    NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
    CALL A FUNCTION THAT MIGHT BLOCK. */  
    
    // emulating the idle task, this will be a thread at the lowest priority
    xTaskCreate(vApplicationIdleHook, (signed portCHAR *)"Idle", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

}
#endif

#if ( configUSE_TICK_HOOK == 1 )
{
    extern void vApplicationTickHook(void);

  // emulating the 1ms tick hook, this uses a timer (the last one available)
  // because this is software, we get lots of timers so this is the best way
  // to fake a 1ms tick
  // sadly, because this is software, there's up to 10mS of jitter. Bah.
  MIOS32_TIMER_Init((NUM_TIMERS-1), 1, vApplicationTickHook, 99);

}
#endif


	
	
}


void vApplicationTickHook(void)
{
// oh look more todo
/*
#ifndef MIOS32_DONT_USE_SRIO
  // notify application about SRIO scan start
  APP_SRIO_ServicePrepare();

  // start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
  MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);
#endif
*/
}


/////////////////////////////////////////////////////////////////////////////
// Idle Hook (called by FreeRTOS when nothing else to do)
/////////////////////////////////////////////////////////////////////////////
void vApplicationIdleHook(void *pvParameters)
{
  // branch endless to application
  // the JUCE threads will ensure that this loops
  // so no need for while (1) {....}
    APP_Background();
}


/////////////////////////////////////////////////////////////////////////////
// Remaining application hooks
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  // the JUCE threads will ensure that this loops
  // so no need for while (1) {....}
  
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

// wow, a todo
// who'd have guessed?
/*
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
    // check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
    MIOS32_DIN_Handler(APP_DIN_NotifyToggle);

    // check for encoder changes, call APP_ENC_NotifyChanged on each change
# ifndef MIOS32_DONT_USE_ENC
    MIOS32_ENC_Handler(APP_ENC_NotifyChange);
# endif
#endif

#if !defined(MIOS32_DONT_USE_AIN)
    // check for AIN pin changes, call APP_AIN_NotifyChange on each pin change
    MIOS32_AIN_Handler(APP_AIN_NotifyChange);
#endif

#if !defined(MIOS32_DONT_USE_MIDI)
#ifndef MIOS32_DONT_USE_USB_MIDI
    // handle USB messages
    MIOS32_USB_MIDI_Handler();
#endif
    
    // check for incoming MIDI messages and call hooks
    MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);
#endif

#if !defined(MIOS32_DONT_USE_COM)
    // check for incoming COM messages and call hook
    MIOS32_COM_Receive_Handler(APP_NotifyReceivedCOM);
#endif
*/

}








// one more todo for the road

// if this emulation is to be complete, it needs to have an emulation 
// for all the MIOS32 and FREERTOS functions....
// 
// Let's just start with what the vX needs
// and we can move on/get contributions later..
// so here's what needs to be done first:



// all this stuff should go in it's own files
// kinda like mios32_timer.c and JUCEtimer.cpp
// and the other emulations


#define dummyreturn 0 // just something to keep the compiler quiet for now




/*
MIOS32_BOARD_LED_Init
MIOS32_BOARD_LED_Set
*/


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

