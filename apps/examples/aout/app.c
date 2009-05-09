// $Id$
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
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

#include <aout.h>

/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

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

#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
  DEBUG_MSG("Settings as configured in Init():\n");
  DEBUG_MSG("  if_type = %d\n", config.if_type);
  DEBUG_MSG("  if_option = 0x%08x\n", config.if_option);
  DEBUG_MSG("  num_channels = 0x%08x\n", config.num_channels);
  DEBUG_MSG("  chn_inverted = 0x%08x\n", config.chn_inverted);
  DEBUG_MSG("\n", config.chn_inverted);
  DEBUG_MSG("MIDI Event Mapping as programmed in APP_NotifyReceivedEvent():\n");
  DEBUG_MSG("  AOUT Channel #1 controlled by MIDI Note at MIDI Channel #1\n");
  DEBUG_MSG("  AOUT Channel #2 controlled by Note Velocity at MIDI Channel #1\n");
  DEBUG_MSG("  AOUT Digital Output (MBHP_AOUT only) controlled by Note at MIDI Channel #1\n");
  DEBUG_MSG("  AOUT Channel #3 controlled by CC#1 at MIDI Channel #1\n");
  DEBUG_MSG("  AOUT Channel #4 controlled by CC#7 at MIDI Channel #1\n");
  DEBUG_MSG("  AOUT Channel #5 controlled by MIDI Note at MIDI Channel #2\n");
  DEBUG_MSG("  AOUT Channel #6 controlled by Note Velocity at MIDI Channel #2\n");
  DEBUG_MSG("  AOUT Digital Output (MBHP_AOUT only) controlled by Note at MIDI Channel #2\n");
  DEBUG_MSG("  AOUT Channel #7 controlled by CC#1 at MIDI Channel #2\n");
  DEBUG_MSG("  AOUT Channel #8 controlled by CC#7 at MIDI Channel #2\n");
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();

  // endless loop
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("Received MIDI Event: %02X %02X %02X\n", 
		midi_package.evnt0, midi_package.evnt1, midi_package.evnt2);
#endif

  // for easier handling: Note Off -> Note On with velocity 0
  if( midi_package.event == NoteOff ) {
    midi_package.event = NoteOn;
    midi_package.velocity = 0;
  }

  // some spagetthi code to simplify understanding and adaptions


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #1 and #2 + Digital Pin #1
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn1 && midi_package.event == NoteOn ) {
    if( midi_package.velocity ) {
      // change voltages
      AOUT_PinSet(0, midi_package.note << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #1 set to %04x\n", midi_package.note << 9);
#endif

      AOUT_PinSet(1, midi_package.velocity << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #2 set to %04x\n", midi_package.velocity << 9);
#endif

      // enable gate (MBHP_AOUT only)
      AOUT_DigitalPinSet(0, 1);

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Digital Pin #1 set to 1\n");
#endif

    } else {
      // disable gate (MBHP_AOUT only)
      AOUT_DigitalPinSet(0, 0);

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Digital Pin #1 set to 0\n");
#endif
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #3
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn1 && midi_package.event == CC && midi_package.cc_number == 1 ) {
    AOUT_PinSet(2, midi_package.value << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #3 set to %04x\n", midi_package.value << 9);
#endif
  }


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #4
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn1 && midi_package.event == CC && midi_package.cc_number == 7 ) {
    AOUT_PinSet(3, midi_package.value << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #4 set to %04x\n", midi_package.value << 9);
#endif
  }


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #5 and #6 + Digital Pin #2
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn2 && midi_package.event == NoteOn ) {
    if( midi_package.velocity ) {
      // change voltages
      AOUT_PinSet(4, midi_package.note << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #5 set to %04x\n", midi_package.note << 9);
#endif

      AOUT_PinSet(5, midi_package.velocity << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #6 set to %04x\n", midi_package.velocity << 9);
#endif

      // enable gate (MBHP_AOUT only)
      AOUT_DigitalPinSet(1, 1);

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Digital Pin #2 set to 1\n");
#endif

    } else {
      // disable gate (MBHP_AOUT only)
      AOUT_DigitalPinSet(1, 0);

#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Digital Pin #2 set to 0\n");
#endif
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #7
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn2 && midi_package.event == CC && midi_package.cc_number == 1 ) {
    AOUT_PinSet(6, midi_package.value << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #7 set to %04x\n", midi_package.value << 9);
#endif
  }


  ///////////////////////////////////////////////////////////////////////////
  // AOUT Channel #8
  ///////////////////////////////////////////////////////////////////////////
  if( midi_package.chn == Chn2 && midi_package.event == CC && midi_package.cc_number == 7 ) {
    AOUT_PinSet(7, midi_package.value << 9); // convert 7bit to 16bit value
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("AOUT Channel #8 set to %04x\n", midi_package.value << 9);
#endif
  }


  // update AOUT channels (if required)
  AOUT_Update();
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
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
