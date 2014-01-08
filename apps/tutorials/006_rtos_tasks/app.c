// $Id$
/*
 * MIOS32 Tutorial #006: RTOS Tasks
 * see README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for J5_SCAN task:
// use same priority as MIOS32 specific tasks (3)
#define PRIORITY_TASK_J5_SCAN	( tskIDLE_PRIORITY + 3 )

// local prototype of the task function
static void TASK_J5_Scan(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize all pins of J5A, J5B and J5C as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<12; ++pin)
    MIOS32_BOARD_J5_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // start task
  xTaskCreate(TASK_J5_Scan, (signed portCHAR *)"J5_Scan", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_J5_SCAN, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
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
}


/////////////////////////////////////////////////////////////////////////////
// This task scans J5 pins periodically
/////////////////////////////////////////////////////////////////////////////
static void TASK_J5_Scan(void *pvParameters)
{
  u8 old_state[12]; // to store the state of 12 pins
  u8 debounce_ctr[12]; // to store debounce delay counters
  portTickType xLastExecutionTime;

  // initialize pin state and debounce counters to inactive value
  int pin;
  for(pin=0; pin<12; ++pin) {
    old_state[pin] = 1;
    debounce_ctr[pin] = 0;
  }

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 1 / portTICK_RATE_MS);

    // toggle Status LED to as a sign of live
    MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

    // check each J5 pin for changes
    int pin;
    for(pin=0; pin<12; ++pin) {
      // debounce counter active?
      if( debounce_ctr[pin] ) {
	// decrement value, wait for another mS
	--debounce_ctr[pin];
      } else {
	// check for new pin state
	u8 new_state = MIOS32_BOARD_J5_PinGet(pin);

	// pin changed?
	if( new_state != old_state[pin] ) {
	  // store new state
	  old_state[pin] = new_state;

	  // // delay next check for 20 mS
	  debounce_ctr[pin] = 20;

	  // send Note On/Off Event depending on new pin state
	  if( new_state == 0 ) // Jumper closed -> 0V
	    MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, 0x3c + pin, 127);
	  else                 // Jumper opened -> ca. 3.3V
	    MIOS32_MIDI_SendNoteOff(DEFAULT, Chn1, 0x3c + pin, 0);
	}
      }
    }
  }
}
