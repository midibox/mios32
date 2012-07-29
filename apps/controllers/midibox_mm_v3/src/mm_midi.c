// $Id$
/*
 * MIDIbox MM V3
 * MIDI Handler
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
#include "mm_hwcfg.h"
#include "mm_midi.h"
#include "mm_mf.h"
#include "mm_vpot.h"
#include "mm_dio.h"
#include "mm_lcd.h"
#include "mm_leddigits.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function is called when a complete
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MM_MIDI_Received(mios32_midi_package_t package)
{
  static u8 fader_pos_l = 0;
  static u8 fader_pos_h = 0;
  static u8 led_id_l = 0;
  static u8 led_id_h = 0;
  u8 mm_chn = (mm_hwcfg_midi_channel-1) & 0x0f;

  // exit if wrong channel
  if( (package.evnt0 & 0x0f) != mm_chn )
    return 0;

  // extract MIDI status (without channel)
  package.evnt0 &= 0xf0;

  // for easier parsing: convert Note Off to Note On with velocity 0
  if( (package.evnt0 & 0xf0) == 0x90 ) {
    package.evnt0 |= 0x10;
    package.evnt2 = 0x00;
  }

  if( package.evnt0 == 0x90 ) { // Note On/Off

    // ping: if 90 00 00 has been received, return 90 00 7F
    if( package.evnt1 == 0x00 && package.evnt2 == 0x00 ) {
      MIOS32_MIDI_SendNoteOn(DEFAULT, mm_chn, 0x00, 0x7f);
    }

  } else if( package.evnt0 == 0xb0) { // CC

    if( (package.evnt1 & 0x78) == 0x00 ) {        // Bn 0[0-7]: Fader MSB
      fader_pos_h = package.evnt2;
    } else if( (package.evnt1 & 0x78) == 0x20 ) { // Bn 2[0-7]: Fader LSB
      fader_pos_l = package.evnt2;
      u16 value_14bit = (fader_pos_h << 7) | (fader_pos_l & 0x7f);
      MM_MF_FaderMove(package.evnt1 & 0x07, value_14bit);
    } else if( package.evnt1 == 0x0c ) {          // Bn 0C: LED ID_H
      led_id_h = package.evnt2;
    } else if( package.evnt1 == 0x2c ) {          // Bn 2C: LED ID_L
      led_id_l = package.evnt2;
      MM_DIO_LEDHandler(led_id_l, led_id_h);
    }

  }

  return 0; // no error

#if 0
  // LED set command? (Note On)
  if( package.type == NoteOn ) {
    MM_DIO_LEDSet(package.note, package.velocity);
    MM_LCD_LEDStatusUpdate(package.note, package.velocity);
    return 0; // no error
  }

  // Fader move command? (Pitch Bender)
  if( package.type == PitchBend ) {

#if MOTORFADER0_IS_MASTERFADER
    if( package.chn == Chn1 )
      return MM_MF_FaderMove(0, pitchbend_value_14bit << 2); // convert to 16bit
#endif
    if( package.chn < 8 )
      return MM_MF_FaderMove(package.chn, pitchbend_value_14bit << 2); // convert to 16bit

    return -1; // invalid channel
  }

  // LEDring command? (B0 3x)
  if( package.type == CC && ((package.cc_number & 0xf8) == 0x30) ) {
    return MM_VPOT_ValueSet(package.cc_number & 0x07, package.value);
  }

  // MTC or STATUS digit?
  // Note: Cubase sends this over channel 16, therefore the channel number is masked out here!
  if( package.type == CC && ((package.cc_number & 0xf0) == 0x40) ) {
    if( package.cc_number >= 0x40 && package.cc_number <= 0x49 ) {
      return MM_LEDDIGITS_MTCSet(9 - (package.cc_number - 0x40), package.value);
    } else if( package.cc_number >= 0x4a && package.cc_number <= 0x4b ) {
      return MM_LEDDIGITS_StatusSet(1 - (package.cc_number - 0x4a), package.value);
    }
    return -1; // invalid digit
  }
#endif

  return -2; // unsupported command
}
