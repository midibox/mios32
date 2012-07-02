// $Id$
/*
 * MIDIbox LC V2
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
#include "lc_hwcfg.h"
#include "lc_midi.h"
#include "lc_mf.h"
#include "lc_vpot.h"
#include "lc_dio.h"
#include "lc_meters.h"
#include "lc_lcd.h"
#include "lc_leddigits.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function is called when a complete
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 LC_MIDI_Received(mios32_midi_package_t package)
{
  // LED set command? (Note On)
  if( package.type == NoteOn ) {
    LC_DIO_LEDSet(package.note, package.velocity);
    LC_LCD_LEDStatusUpdate(package.note, package.velocity);
    return 0; // no error
  }

  // In addition we are switching off the LED on Note Off 
  // (this is not part of the LC spec, but useful if LC LEDs are tested with a MIDI keyboard)
  if( package.type == NoteOff ) {
    LC_DIO_LEDSet(package.note, 0);
    LC_LCD_LEDStatusUpdate(package.note, 0);
    return 0; // no error
  }

  // Fader move command? (Pitch Bender)
  if( package.type == PitchBend ) {
    u16 pitchbend_value_14bit = (package.evnt2 << 7) | (package.evnt1 & 0x7f);

#if MOTORFADER0_IS_MASTERFADER
    if( package.chn == Chn1 )
      return LC_MF_FaderMove(0, pitchbend_value_14bit << 2); // convert to 16bit
#endif
    if( package.chn < 8 )
      return LC_MF_FaderMove(package.chn, pitchbend_value_14bit << 2); // convert to 16bit

    return -1; // invalid channel
  }

  // LEDring command? (B0 3x)
  if( package.type == CC && ((package.cc_number & 0xf8) == 0x30) ) {
    return LC_VPOT_ValueSet(package.cc_number & 0x07, package.value);
  }

  // MTC or STATUS digit?
  // Note: Cubase sends this over channel 16, therefore the channel number is masked out here!
  if( package.type == CC && ((package.cc_number & 0xf0) == 0x40) ) {
    if( package.cc_number >= 0x40 && package.cc_number <= 0x49 ) {
      return LC_LEDDIGITS_MTCSet(9 - (package.cc_number - 0x40), package.value);
    } else if( package.cc_number >= 0x4a && package.cc_number <= 0x4b ) {
      return LC_LEDDIGITS_StatusSet(1 - (package.cc_number - 0x4a), package.value);
    }
    return -1; // invalid digit
  }

  // Meter set command? (Aftertouch)
  if( package.type == Aftertouch ) {
    return LC_METERS_LevelSet(package.note >> 4, package.note & 0x0f);
  }

  return -2; // unsupported command
}
