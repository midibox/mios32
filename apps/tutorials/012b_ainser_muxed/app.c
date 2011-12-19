// $Id$
/*
 * MIOS32 Tutorial #012b: Scanning up to 64 analog pots from an AINSER module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <ainser.h>
#include "app.h"


// include everything FreeRTOS related
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for AINSER_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_AINSER_SCAN ( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_AINSER_Scan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize the AINSER module(s)
  AINSER_Init(0);

  // start task
  xTaskCreate(TASK_AINSER_Scan, (signed portCHAR *)"AINSER_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_AINSER_SCAN, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  // MIOS32_AIN not used here!!!
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
static void APP_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value)
{
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // convert 12bit value to 7bit value
  u8 value_7bit = pin_value >> 5;

  // send MIDI event
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, pin + 0x10, value_7bit);
}


/////////////////////////////////////////////////////////////////////////////
// This task scans AINSER pins and checks for updates
/////////////////////////////////////////////////////////////////////////////
static void TASK_AINSER_Scan(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // scan pins
    AINSER_Handler(APP_AINSER_NotifyChange);
  }
}

