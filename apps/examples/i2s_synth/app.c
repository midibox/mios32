// $Id$
/*
 * Demo application for I2S audio
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
#include "synth.h"

#include <FreeRTOS.h>
#include <portmacro.h>

#include "frqtab.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

volatile u8 print_msg;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  // init Synth
  SYNTH_Init(0);

  // print first message
  print_msg = PRINT_MSG_INIT;
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

    // new message requested?
    // TODO: add FreeRTOS specific queue handling!
    u8 new_msg = PRINT_MSG_NONE;

    portENTER_CRITICAL(); // port specific FreeRTOS function to disable IRQs (nested)
    if( print_msg ) {
      new_msg = print_msg;
      print_msg = PRINT_MSG_NONE; // clear request
    }
    portEXIT_CRITICAL(); // port specific FreeRTOS function to enable IRQs (nested)

    switch( new_msg ) {
      case PRINT_MSG_INIT:
        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintFormattedString("see README.txt   ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintFormattedString("for details     ");
	break;

      case PRINT_MSG_SELECTIONS: {
	// waveform names
	const u8 waveform_name[SYNTH_NUM_WAVEFORMS][4] = { "Tri", "Saw", "Pul", "Sin" };

        MIOS32_LCD_CursorSet(0, 0);
        MIOS32_LCD_PrintFormattedString("     L    R     ");
        MIOS32_LCD_CursorSet(0, 1);
        MIOS32_LCD_PrintFormattedString("    %s  %s    ", 
					waveform_name[SYNTH_WaveformGet(0)],
					waveform_name[SYNTH_WaveformGet(1)]);
        }
	break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // if note event over MIDI channel #1 controls note of both oscillators
  if( midi_package.event == NoteOn && midi_package.chn == Chn1 && midi_package.velocity > 0 ) {
    int chn;
    for(chn=0; chn<2; ++chn)
      SYNTH_FrequencySet(chn, frqtab[midi_package.note]);
  }

  // CC#1 over MIDI channel #1 controls waveform
  if( midi_package.event == CC && midi_package.chn == Chn1 ) {
    int chn;
    for(chn=0; chn<2; ++chn)
      SYNTH_WaveformSet(chn, midi_package.value >> 5);
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
  u32 tmp;

  // ignore if button depressed
  if( pin_value )
    return;

  // call function depending on button number
  switch( pin ) {
    case DIN_NUMBER_EXEC: // Exec button
      break;

    case DIN_NUMBER_INC: // Inc button
      // increment right channel waveform
      tmp = SYNTH_WaveformGet(1);
      if( ++tmp >= SYNTH_NUM_WAVEFORMS )
	tmp = 0;
      SYNTH_WaveformSet(1, tmp);
      print_msg = PRINT_MSG_SELECTIONS;
      break;

    case DIN_NUMBER_DEC: // Dec button
      // increment left channel waveform
      tmp = SYNTH_WaveformGet(0);
      if( ++tmp >= SYNTH_NUM_WAVEFORMS )
	tmp = 0;
      SYNTH_WaveformSet(0, tmp);
      print_msg = PRINT_MSG_SELECTIONS;
      break;

    case DIN_NUMBER_SNAPSHOT: // Snapshot button
      break;
  }
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
