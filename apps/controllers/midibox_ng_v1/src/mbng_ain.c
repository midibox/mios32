// $Id$
/*
 * AIN access functions for MIDIbox NG
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

#include "mbng_ain.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MBNG_AIN_SendMIDIEvent(mbng_patch_ain_entry_t *ain_cfg, u8 value_7bit);


/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_AIN_NotifyChange when an input
// has changed its value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_AIN )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MBNG_AIN_NotifyChange(%d, %d)\n", pin, pin_value);
#endif

  // convert 12bit value to 7bit value
  u8 value_7bit = pin_value >> 5;

  // get pin configuration from patch structure
  mbng_patch_ain_entry_t *ain_cfg = (mbng_patch_ain_entry_t *)&mbng_patch_ain[pin];

  return MBNG_AIN_SendMIDIEvent(ain_cfg, value_7bit);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange_SER64(u32 module, u32 pin, u32 pin_value)
{
  int mbng_pin = module*64 + pin;
  if( mbng_pin >= MBNG_PATCH_NUM_AINSER )
    return -1; // invalid pin

#if 0
  DEBUG_MSG("MBNG_AIN_NotifyChange_SER64(%d, %d, %d)\n", module, pin, pin_value);
#endif

  // convert 12bit value to 7bit value
  u8 value_7bit = pin_value >> 5;

  // get pin configuration from patch structure
  mbng_patch_ain_entry_t *ain_cfg = (mbng_patch_ain_entry_t *)&mbng_patch_ainser[mbng_pin];

  return MBNG_AIN_SendMIDIEvent(ain_cfg, value_7bit);
}


/////////////////////////////////////////////////////////////////////////////
// This function sends the MIDI event based on AIN pin configuration
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_AIN_SendMIDIEvent(mbng_patch_ain_entry_t *ain_cfg, u8 value_7bit)
{
  // silently ignore if pin not mapped (default)
  if( !ain_cfg->enabled_ports )
    return 0;

  // determine MIDI event type
  mios32_midi_event_t event = (ain_cfg->evnt0 >> 4) | 0x8;

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = event;
  p.event = event;

  if( mbng_patch_cfg.global_chn )
    p.chn = mbng_patch_cfg.global_chn - 1;
  else
    p.chn = ain_cfg->evnt0;

  p.evnt1 = ain_cfg->evnt1;
  p.evnt2 = value_7bit;

  // send MIDI package over enabled ports
  int i;
  u16 mask = 1;
  for(i=0; i<16; ++i, mask <<= 1) {
    if( ain_cfg->enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}
