// $Id$
/*
 * DIN access functions for MIDIO128 V3
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

#include "midio_din.h"
#include "midio_dout.h"
#include "midio_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u32 toggle_flags[MIDIO_PATCH_NUM_DOUT/32];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MIDIO_PATCH_NUM_DOUT/32; ++i)
    toggle_flags[i] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_DIN_NotifyToggle when an input
// has been toggled
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  if( pin >= MIDIO_PATCH_NUM_DIN )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MIDIO_DIN_NotifyToggle(%d, %d)\n", pin, pin_value);
#endif

  // get pin configuration from patch structure
  midio_patch_din_entry_t *din_cfg = (midio_patch_din_entry_t *)&midio_patch_din[pin];

  // button depressed? (take INVERSE_DIN flag into account)
  // note: on a common configuration (MBHP_DINX4 module used with pull-ups), pins are inverse
  u8 depressed = pin_value ? 0 : 1;
  if( midio_patch_cfg.flags.INVERSE_DIN )
    depressed ^= 1;

  // toggle mode?
  if( din_cfg->mode == MIDIO_PATCH_DIN_MODE_TOGGLE ) {
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
  if( midio_patch_cfg.flags.FORWARD_IO )
    MIDIO_DOUT_PinSet(pin, depressed ? 0 : 1);

  // in ON only mode: don't send MIDI event if button depressed
  if( din_cfg->mode == MIDIO_PATCH_DIN_MODE_ON_ONLY && depressed )
    return 0;

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = event;
  p.event = event;

  if( midio_patch_cfg.global_chn )
    p.chn = midio_patch_cfg.global_chn - 1;
  else
    p.chn = (depressed ? din_cfg->evnt0_off : din_cfg->evnt0_on);

  p.evnt1 = (depressed ? din_cfg->evnt1_off : din_cfg->evnt1_on);
  p.evnt2 = (depressed ? din_cfg->evnt2_off : din_cfg->evnt2_on);

  // send MIDI package over enabled ports
  int i;
  int mask = 1;
  for(i=0; i<8; ++i, mask <<= 1) {
    if( din_cfg->enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = 0x10 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}
