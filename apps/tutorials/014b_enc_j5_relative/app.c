// $Id$
/*
 * MIOS32 Tutorial #014b: Sending relative MIDI events with rotary encoders connected to J5
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


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// encoders connected to J5A/B/C
#define NUM_ENCODERS 6


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

  // initialize rotary encoders of the same type (DETENTED2)
  int enc;
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
    mios32_enc_config_t enc_config = MIOS32_ENC_ConfigGet(enc);
    enc_config.cfg.type = DETENTED2; // see mios32_enc.h for available types
    enc_config.cfg.sr = 0; // must be 0 if controlled from application
    enc_config.cfg.pos = 0; // doesn't matter if controlled from application
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(enc, enc_config);
  }
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
  // get current J5 value and update all encoder states
  int enc;
  u16 state = MIOS32_BOARD_J5_Get();
  for(enc=0; enc<NUM_ENCODERS; ++enc) {
    MIOS32_ENC_StateSet(enc, state & 3); // two pins
    state >>= 2; // shift to next two pins
  }
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
  // toggle Status LED on each AIN value change
  MIOS32_BOARD_LED_Set(0x0001, ~MIOS32_BOARD_LED_Get());

  // determine relative value: 64 +/- <incrementer>
  int value = 64 + incrementer;

  // ensure that value is in range of 0..127
  if( value < 0 )
    value = 0;
  else if( value > 127 )
    value = 127;

  // send event
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, 0x10 + encoder, value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
