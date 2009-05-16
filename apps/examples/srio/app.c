// $Id$
/*
 * Demo application for MIOS32_SRIO, DIN, DOUT and ENC driver
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
#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Encoder Configuration
/////////////////////////////////////////////////////////////////////////////
#define NUM_ENCODERS 8
const mios32_enc_config_t encoders[NUM_ENCODERS] = {
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=12, .cfg.pos=6 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=0 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=2 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=4 },
  { .cfg.type=NON_DETENTED, .cfg.speed=NORMAL, .cfg.speed_par=0, .cfg.sr=13, .cfg.pos=6 },
};


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

u8 last_din_pin = 0;
u8 last_din_value = 1;
u8 last_enc = 0;
u8 last_enc_dir = 0;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  s32 i;

  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // initialize encoders
  for(i=0; i<NUM_ENCODERS; ++i)
    MIOS32_ENC_ConfigSet(i, encoders[i]);


  // send welcome message to MIOS terminal
#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
  DEBUG_MSG("Play MIDI Notes over Channel #1 or press button\n");
  DEBUG_MSG("to control the LEDs.\n");
  DEBUG_MSG("\n");
#endif
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  // init LCD
  MIOS32_LCD_Clear();

  // endless loop: print status information on LCD
  while( 1 ) {
    // toggle the state of all LEDs (allows to measure the execution speed with a scope)
    MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
    
    // print text on LCD screen
    MIOS32_LCD_CursorSet(0, 0);
    MIOS32_LCD_PrintFormattedString("DIN Pin #%3d %c", last_din_pin, last_din_value ? 'o' : '*');

    MIOS32_LCD_CursorSet(0, 1);
    MIOS32_LCD_PrintFormattedString("Enc. #%2d (%s)", last_enc, last_enc_dir ? "Right" : "Left ");
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

  // if note event over MIDI channel #1 has been received, toggle appr. DOUT pin
  // change note off events to note on with velocity 0 for easier handling
  if( midi_package.event == NoteOff ) {
    midi_package.event  = NoteOn;
    midi_package.velocity = 0x00;
  }

  // note event over channel #1 sets a DOUT pin
  if( midi_package.chn == Chn1 && midi_package.event == NoteOn )
    MIOS32_DOUT_PinSet(midi_package.note, midi_package.velocity);
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
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("APP_DIN_NotifyToggle(%d, %d)\n", pin, pin_value);
#endif

  // remember pin and value
  last_din_pin = pin;
  last_din_value = pin_value;

  // set DOUT pin
  MIOS32_DOUT_PinSet(pin, pin_value ? 0 : 1);

  // send MIDI event
  MIOS32_MIDI_SendNoteOn(DEFAULT, Chn1, pin, pin_value ? 0x00 : 0x7f);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("APP_ENC_NotifyChange(%d, %d)\n", encoder, incrementer);
#endif

  // remember last pin and direction
  last_enc = encoder;
  last_enc_dir = (incrementer > 0) ? 1 : 0;

  // send MIDI event
  MIOS32_MIDI_SendCC(DEFAULT, Chn1, encoder, incrementer & 0x7f);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
