// $Id$
/*
 * MIDIbox SEQ V4
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

#include <stm32f10x_lib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <seq_demo.h>

/////////////////////////////////////////////////////////////////////////////
// Task Priorities
/////////////////////////////////////////////////////////////////////////////

#define PRIORITY_TASK_DIN_CHECK		( tskIDLE_PRIORITY + 2 )
#define PRIORITY_TASK_MIDI_RECEIVE	( tskIDLE_PRIORITY + 2 )


/////////////////////////////////////////////////////////////////////////////
// Prototypes
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

  // init MBSEQ core
  Init(0);

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
void vApplicationTickHook( void )
{
  // start next SRIO scan - no IRQ based notification required
  MIOS32_SRIO_ScanStart(NULL);
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

    // check for pin changes, call DIN_NotifyToggle on each toggled pin
    // routine located in MBSEQ core
    MIOS32_DIN_Handler(DIN_NotifyToggle);
  }
}


/////////////////////////////////////////////////////////////////////////////
// MIDI Handlers
/////////////////////////////////////////////////////////////////////////////

// this hook is called on received MIDI events
void MIDI_NotifyReceivedEvent(u8 port, mios32_midi_package_t midi_package)
{
  // ignore
}

// this hook is called if SysEx data is received
void MIDI_NotifyReceivedSysEx(u8 port, u8 sysex_byte)
{
  // ignore
}

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
    MIOS32_MIDI_Receive_Handler(MIDI_NotifyReceivedEvent, MIDI_NotifyReceivedSysEx);
  }
}
