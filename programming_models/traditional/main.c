// $Id: main.c 42 2008-09-30 21:49:43Z tk $
/*
 * Traditional Programming Model
 * Provides similar hooks like PIC based MIOS8 to the application
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <app.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>


/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_HOOKS		( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters);
static void TASK_MIDI_Hooks(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  // initialize hardware and MIOS32 modules
#ifndef MIOS32_DONT_USE_SYS
  MIOS32_SYS_Init(0);
#endif
#ifndef MIOS32_DONT_USE_DELAY
  MIOS32_DELAY_Init(0);
#endif
#ifndef MIOS32_DONT_USE_BOARD
  MIOS32_BOARD_Init(0);
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
#ifndef MIOS32_DONT_USE_MIDI
  MIOS32_MIDI_Init(0);
#endif
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

  // initialize application
  APP_Init();

#if MIOS32_LCD_BOOT_MSG_DELAY
  // print boot message
# ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_PrintBootMessage();
# endif

  // wait for given delay (usually 2 seconds)
# ifndef MIOS32_DONT_USE_DELAY
  int delay = 0;
  for(delay=0; delay<MIOS32_LCD_BOOT_MSG_DELAY; ++delay)
    MIOS32_DELAY_Wait_uS(1000);
# endif
#endif

  // start the task which calls the application hooks
  xTaskCreate(TASK_Hooks, (signed portCHAR *)"Hooks", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_HOOKS, NULL);
#if !defined(MIOS32_DONT_USE_MIDI)
  xTaskCreate(TASK_MIDI_Hooks, (signed portCHAR *)"Hooks", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_HOOKS, NULL);
#endif

  // start the scheduler
  vTaskStartScheduler();

  // Will only get here if there was not enough heap space to create the idle task
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Application Tick Hook (called by FreeRTOS each mS)
/////////////////////////////////////////////////////////////////////////////
void SRIO_ServiceFinish(void)
{
#ifndef MIOS32_DONT_USE_SRIO

# ifndef MIOS32_DONT_USE_ENC
  // update encoder states
  MIOS32_ENC_UpdateStates();
# endif

  // notify application about finished SRIO scan
  APP_SRIO_ServiceFinish();
#endif
}

void vApplicationTickHook(void)
{
#ifndef MIOS32_DONT_USE_SRIO
  // notify application about SRIO scan start
  APP_SRIO_ServicePrepare();

  // start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
  MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Idle Hook (called by FreeRTOS when nothing else to do)
/////////////////////////////////////////////////////////////////////////////
void vApplicationIdleHook(void)
{
  // branch endless to application
  while( 1 ) {
    APP_Background();
  }
}


/////////////////////////////////////////////////////////////////////////////
// MIDI task (separated from TASK_Hooks() to ensure parallel handling of
// MIDI events if a hook in TASK_Hooks() blocks)
/////////////////////////////////////////////////////////////////////////////
#if !defined(MIOS32_DONT_USE_MIDI)
static void TASK_MIDI_Hooks(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // handle timeout/expire counters and USB packages
    MIOS32_MIDI_Periodic_mS();

    // check for incoming MIDI messages and call hooks
    MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// Remaining application hooks
/////////////////////////////////////////////////////////////////////////////
static void TASK_Hooks(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

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

#if !defined(MIOS32_DONT_USE_COM)
    // check for incoming COM messages and call hook
    MIOS32_COM_Receive_Handler(APP_NotifyReceivedCOM);
#endif
  }
}
