// $Id$
/*
 * DOUT access functions for MIDIO128 V3
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

#include "midio_dout.h"
#include "midio_patch.h"


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DOUT handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DOUT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // set all DOUT pins depending on polarity
  int pin;
  u8 pin_value = midio_patch_cfg.flags.INVERSE_DOUT;
  for(pin=0; pin<MIDIO_PATCH_NUM_DOUT; ++pin)
    MIOS32_DOUT_PinSet(pin, pin_value);
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_MIDI_NotifyPackage whenver a new
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DOUT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // check for "all notes off" command
  if( midi_package.event == CC &&
      midio_patch_cfg.all_notes_off_chn &&
      (midi_package.chn == (midio_patch_cfg.all_notes_off_chn - 1)) &&
      midi_package.cc_number == 123 ) {
    MIDIO_DOUT_Init(0);
  }      

  // get first DOUT entry from patch structure
  midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[0];

  // create port mask
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0x30)>>2;
  u16 port_mask = subport_mask << port_class;

  // check for matching pins
  int pin;
  for(pin=0; pin<MIDIO_PATCH_NUM_DOUT; ++pin, ++dout_cfg) {
    // check if port is enabled
    if( port == DEFAULT || (dout_cfg->enabled_ports & port_mask) ) {
      // check for matching MIDI event
      if( ((midi_package.evnt0 ^ dout_cfg->evnt0) & 0x7f) == 0 &&
	  midi_package.evnt1 == dout_cfg->evnt1 ) {
	// set pin
	MIDIO_DOUT_PinSet(pin, (midi_package.evnt2 >= 0x40) ? 1 : 0);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function sets a DOUT pin with configured polarity (and mode)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DOUT_PinSet(u32 pin, u32 pin_value)
{
  if( pin >= MIDIO_PATCH_NUM_DOUT )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MIDIO_DOUT_PinSet(%d, %d)\n", pin, pin_value);
#endif

  // DOUT assigned to program change?
  if( midio_patch_cfg.flags.ALT_PROGCHNG &&
      ((midio_patch_dout[pin].evnt0 >> 4) | 0x8) == ProgramChange ) {

    if( !pin_value )
      return 0; // ignore

    // set pin
    if( midio_patch_cfg.flags.INVERSE_DOUT )
      pin_value = pin_value ? 0 : 1;

    MIOS32_DOUT_PinSet(pin, pin_value);

    // search for DOUTs assigned to program change and same channel
    int i;
    midio_patch_dout_entry_t *dout_cfg = (midio_patch_dout_entry_t *)&midio_patch_dout[0];
    u8 search_event = midio_patch_dout[pin].evnt0;
    for(i=0; i<MIDIO_PATCH_NUM_DOUT; ++i, ++dout_cfg) {
      if( i != pin && dout_cfg->evnt0 == search_event )
	MIOS32_DOUT_PinSet(i, midio_patch_cfg.flags.INVERSE_DOUT ? 1 : 0);
    }
  } else {
    if( midio_patch_cfg.flags.INVERSE_DOUT )
      pin_value = pin_value ? 0 : 1;

    MIOS32_DOUT_PinSet(pin, pin_value);
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Should be called on patch configuration changes to invert DOUT pins if
// required
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DOUT_UpdatePolarity(u8 old_polarity, u8 new_polarity)
{
  if( old_polarity != new_polarity ) {
    int pin;
    for(pin=0; pin<MIDIO_PATCH_NUM_DOUT; ++pin)
      MIOS32_DOUT_PinSet(pin, MIOS32_DOUT_PinGet(pin) ? 0 : 1);
  }

  return 0; // no error
}
