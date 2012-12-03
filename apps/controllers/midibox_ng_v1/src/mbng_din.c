// $Id$
/*
 * DIN access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u32 toggle_flags[MBNG_PATCH_NUM_DIN/32];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_DIN/32; ++i)
    toggle_flags[i] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_DIN_NotifyToggle when an input
// has been toggled
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_DIN )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MBNG_DIN_NotifyToggle(%d, %d)\n", pin, pin_value);
#endif

  // get pin configuration from patch structure
  mbng_patch_din_entry_t *din_cfg = (mbng_patch_din_entry_t *)&mbng_patch_din[pin];

  // button depressed? (take INVERSE_DIN flag into account)
  // note: on a common configuration (MBHP_DINX4 module used with pull-ups), pins are inverse
  u8 depressed = pin_value ? 0 : 1;
  if( mbng_patch_cfg.flags.INVERSE_DIN )
    depressed ^= 1;

  // toggle mode?
  if( din_cfg->mode == MBNG_PATCH_DIN_MODE_TOGGLE ) {
    if( depressed )
      return 0;

    int ix = pin / 32;
    int mask = (1 << (pin % 32));
    toggle_flags[ix] ^= mask;
    depressed = (toggle_flags[ix] & mask) ? 0 : 1;
  }

  // pin function disabled?
  if( din_cfg->evnt0_on == 0x7f && din_cfg->evnt1_on == 0x7f && din_cfg->evnt2_on == 0x7f )
    return 0;

  // determine MIDI event type
  mios32_midi_event_t event = ((depressed ? din_cfg->evnt0_off : din_cfg->evnt0_on) >> 4) | 0x8;

  // forward to DOUT if enabled
  if( mbng_patch_cfg.flags.FORWARD_IO )
    MBNG_DOUT_PinSet(pin, depressed ? 0 : 1);

  // in ON only mode: don't send MIDI event if button depressed
  if( din_cfg->mode == MBNG_PATCH_DIN_MODE_ON_ONLY && depressed )
    return 0;

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = event;
  p.event = event;

  if( mbng_patch_cfg.global_chn )
    p.chn = mbng_patch_cfg.global_chn - 1;
  else
    p.chn = (depressed ? din_cfg->evnt0_off : din_cfg->evnt0_on);

  p.evnt1 = (depressed ? din_cfg->evnt1_off : din_cfg->evnt1_on);
  p.evnt2 = (depressed ? din_cfg->evnt2_off : din_cfg->evnt2_on);

  // send MIDI package over enabled ports
  int i;
  u16 mask = 1;
  for(i=0; i<16; ++i, mask <<= 1) {
    if( din_cfg->enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}
