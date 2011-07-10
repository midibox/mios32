// $Id$
/*
 * MIOS32 Tutorial #027: Standard Control Surface
 * see README.txt for details
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
#include "app.h"

// include source of the SCS
#include <scs.h>
#include "scs_config.h"

#include "cc_labels.h"

// include everything FreeRTOS related we don't understand yet ;)
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>


// define priority level for control surface handler
// use lower priority as MIOS32 specific tasks (2), so that slow LCDs don't affect overall performance
#define PRIORITY_TASK_PERIOD_1mS_LP ( tskIDLE_PRIORITY + 2 )

// local prototype of the task function
static void TASK_Period_1mS_LP(void *pvParameters);


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize CC label module
  CC_LABELS_Init(0);

  // initialize all J10 pins as inputs with internal Pull-Up
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, MIOS32_BOARD_PIN_MODE_INPUT_PU);

  // initialize Standard Control Surface
  SCS_Init(0);

  // initialize local SCS configuration
  SCS_CONFIG_Init(0);

  // start task
  xTaskCreate(TASK_Period_1mS_LP, (signed portCHAR *)"1mS_LP", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_PERIOD_1mS_LP, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // toggle Status LED to as a sign of life
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());
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
#ifdef MIOS32_FAMILY_LPC17xx
  // pass current pin state of J10
  // only available for LPC17xx, all others (like STM32) default to SRIO
  SCS_AllPinsSet(MIOS32_BOARD_J10_Get());
#endif

  // update encoders/buttons of SCS
  SCS_EncButtonUpdate_Tick();
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
  // for STM32 (no J10 available, accordingly encoder/buttons connected to DIN):
  // if DIN pin number between 0..7: pass to SCS
  if( pin < 8 )
    SCS_DIN_NotifyToggle(pin, pin_value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  // pass encoder event to SCS handler
  if( encoder == SCS_ENC_MENU_ID )
    SCS_ENC_MENU_NotifyChange(incrementer);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This task handles the control surface
/////////////////////////////////////////////////////////////////////////////
static void TASK_Period_1mS_LP(void *pvParameters)
{
  MIOS32_LCD_Clear();

  while( 1 ) {
    vTaskDelay(1 / portTICK_RATE_MS);

    // call SCS handler
    SCS_Tick();
  }
}

