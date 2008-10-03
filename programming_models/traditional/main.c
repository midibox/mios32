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
#define PRIORITY_TASK_MIDI_RECEIVE	( tskIDLE_PRIORITY + 3 )


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void TASK_DIN_Check(void *pvParameters);
static void TASK_MIDI_Receive(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // initialize hardware and MIOS32 modules
  MIOS32_SYS_Init(0);
  MIOS32_SRIO_Init(0);
  MIOS32_DIN_Init(0);
  MIOS32_DOUT_Init(0);
  MIOS32_MIDI_Init(0); // 0 = blocking mode
  MIOS32_LCD_Init(0);

  // initialize application
  APP_Init();

  // start the tasks
  xTaskCreate(TASK_DIN_Check,  (signed portCHAR *)"DIN_Check", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DIN_CHECK, NULL);
  xTaskCreate(TASK_MIDI_Receive, (signed portCHAR *)"MIDI_Receive", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_MIDI_RECEIVE, NULL);

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
  // notify application about finished SRIO scan
  APP_SRIO_ServiceFinish();
}

void vApplicationTickHook(void)
{
  // notify application about SRIO scan start
  APP_SRIO_ServicePrepare();

  // start next SRIO scan - IRQ notification to SRIO_ServiceFinish()
  MIOS32_SRIO_ScanStart(SRIO_ServiceFinish);
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
// checks for toggled DIN pins
static void TASK_DIN_Check(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // check for pin changes, call APP_DIN_NotifyToggle on each toggled pin
    MIOS32_DIN_Handler(APP_DIN_NotifyToggle);
  }
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Handlers
/////////////////////////////////////////////////////////////////////////////

// checks for incoming MIDI messages
static void TASK_MIDI_Receive(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // handle USB messages
    MIOS32_USB_Handler();
    
    // check for incoming MIDI messages and call hooks
    MIOS32_MIDI_Receive_Handler(APP_NotifyReceivedEvent, APP_NotifyReceivedSysEx);
  }
}
