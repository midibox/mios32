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

#define PRIORITY_TASK_DIN_CHECK		( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_AIN_CHECK		( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_MIDI_RECEIVE	( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TASK_COM_RECEIVE	( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
static void TASK_DIN_Check(void *pvParameters);
#endif

#if !defined(MIOS32_DONT_USE_AIN)
static void TASK_AIN_Check(void *pvParameters);
#endif

#ifndef MIOS32_DONT_USE_MIDI
static void TASK_MIDI_Receive(void *pvParameters);
#endif

#ifndef MIOS32_DONT_USE_COM
static void TASK_COM_Receive(void *pvParameters);
#endif


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // initialize hardware and MIOS32 modules
#ifndef MIOS32_DONT_USE_SYS
  MIOS32_SYS_Init(0);
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
  MIOS32_MIDI_Init(0); // 0 = blocking mode
#endif
#ifndef MIOS32_DONT_USE_COM
  MIOS32_COM_Init(0); // 0 = blocking mode
#endif
#ifndef MIOS32_DONT_USE_LCD
  MIOS32_LCD_Init(0);
#endif

  // initialize application
  APP_Init();

  // start the tasks
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
  xTaskCreate(TASK_DIN_Check,  (signed portCHAR *)"DIN_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DIN_CHECK, NULL);
#endif
#if !defined(MIOS32_DONT_USE_AIN)
  xTaskCreate(TASK_AIN_Check,  (signed portCHAR *)"AIN_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_AIN_CHECK, NULL);
#endif
#if !defined(MIOS32_DONT_USE_MIDI)
  xTaskCreate(TASK_MIDI_Receive, (signed portCHAR *)"MIDI_Receive", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI_RECEIVE, NULL);
#endif
#if !defined(MIOS32_DONT_USE_COM)
  xTaskCreate(TASK_COM_Receive, (signed portCHAR *)"COM_Receive", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_COM_RECEIVE, NULL);
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
// DIN Handler
/////////////////////////////////////////////////////////////////////////////
#if !defined(MIOS32_DONT_USE_DIN) && !defined(MIOS32_DONT_USE_SRIO)
static void TASK_DIN_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for DIN pin changes, call APP_DIN_NotifyToggle on each toggled pin
    MIOS32_DIN_Handler(APP_DIN_NotifyToggle);

    // check for encoder changes, call APP_ENC_NotifyChanged on each change
# ifndef MIOS32_DONT_USE_ENC
    MIOS32_ENC_Handler(APP_ENC_NotifyChange);
# endif
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// AIN Handler
/////////////////////////////////////////////////////////////////////////////
#if !defined(MIOS32_DONT_USE_AIN)
static void TASK_AIN_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for AIN pin changes, call APP_AIN_NotifyChange on each pin change
    MIOS32_AIN_Handler(APP_AIN_NotifyChange);
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// MIDI Handlers
/////////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_MIDI
// checks for incoming MIDI messages
static void TASK_MIDI_Receive(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

#ifndef MIOS32_DONT_USE_USB_MIDI
    // handle USB messages
    MIOS32_USB_MIDI_Handler();
#endif
    
    // check for incoming MIDI messages and call hooks
    MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);
  }
}
#endif


/////////////////////////////////////////////////////////////////////////////
// MIDI Handlers
/////////////////////////////////////////////////////////////////////////////
#ifndef MIOS32_DONT_USE_COM
// checks for incoming COM messages
static void TASK_COM_Receive(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for incoming COM messages and call hook
    MIOS32_COM_Receive_Handler(APP_NotifyReceivedCOM);
  }
}
#endif
