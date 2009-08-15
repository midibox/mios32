// $Id$
/*
 * MIOS32 Tutorial #016: Using AOUTs and a Notestack
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

#include <notestack.h>
#include <aout.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NOTESTACK_SIZE 16


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // turn off gate LED
  MIOS32_BOARD_LED_Set(1, 0);

  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);

  // initialize AOUT module
  AOUT_Init(0);

  // configure interface
  // see AOUT module documentation for available interfaces and options
  aout_config_t config;
  config = AOUT_ConfigGet();
  config.if_type = AOUT_IF_MAX525;
  config.if_option = 0;
  config.num_channels = 8;
  config.chn_inverted = 0;
  AOUT_ConfigSet(config);
  AOUT_IF_Init(0);
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
  // Note On received?
  if( midi_package.chn == Chn1 && 
      (midi_package.type == NoteOn || midi_package.type == NoteOff) ) {

    // branch depending on Note On/Off event
    if( midi_package.event == NoteOn && midi_package.velocity > 0 ) {
      // push note into note stack
      NOTESTACK_Push(&notestack, midi_package.note, midi_package.velocity);
    } else {
      // remove note from note stack
      NOTESTACK_Pop(&notestack, midi_package.note);
    }


    // still a note in stack?
    if( notestack.len ) {
      // take first note of stack
      // we have to convert the 7bit value to a 16bit value
      u16 note_cv = notestack_items[0].note << 9;
      u16 velocity_cv = notestack_items[0].tag << 9;

      // change voltages
      AOUT_PinSet(0, note_cv);
      AOUT_PinSet(1, velocity_cv);

      // set board LED
      MIOS32_BOARD_LED_Set(1, 1);

      // set AOUT digital output (if available)
      AOUT_DigitalPinSet(0, 1);
    } else {
      // turn off LED (can also be used as a gate output!)
      MIOS32_BOARD_LED_Set(1, 0);

      // clear AOUT digital output (if available)
      AOUT_DigitalPinSet(0, 0);
    }

    // update AOUT channel(s)
    AOUT_Update();

#if 1
    // optional debug messages
    NOTESTACK_SendDebugMessage(&notestack);
#endif

  // CC received?
  } else if( midi_package.chn == Chn1 && midi_package.type == CC ) {

    // we have to convert the 7bit value to a 16bit value
    u16 cc_cv = midi_package.value << 9;

    switch( midi_package.cc_number ) {
      case 16: AOUT_PinSet(2, cc_cv); break;
      case 17: AOUT_PinSet(3, cc_cv); break;
      case 18: AOUT_PinSet(4, cc_cv); break;
      case 19: AOUT_PinSet(5, cc_cv); break;
      case 20: AOUT_PinSet(6, cc_cv); break;
      case 21: AOUT_PinSet(7, cc_cv); break;
    }

    // update AOUT channel(s)
    // register upload will be omitted automatically if no channel value has changed
    AOUT_Update();
  }
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
