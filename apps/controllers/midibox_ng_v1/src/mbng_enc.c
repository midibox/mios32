// $Id$
/*
 * Encoder access functions for MIDIbox NG
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

#include "mbng_enc.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 enc_group;
static u16 enc_value[MBNG_PATCH_NUM_ENC];

/////////////////////////////////////////////////////////////////////////////
// This function initializes the ENC handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  enc_group = 0;

  int i;
  for(i=0; i<MBNG_PATCH_NUM_ENC; ++i)
    enc_value[i] = 0;
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set the selected encoder group
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_GroupGet(void)
{
  return enc_group;
}

s32 MBNG_ENC_GroupSet(u8 new_group)
{
  enc_group = new_group;
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_ENC_NotifyChange when an encoder
// has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
  if( encoder >= MBNG_PATCH_NUM_ENC )
    return -1; // invalid encoder

#if 0
  DEBUG_MSG("MBNG_ENC_NotifyChange(%d, %d)\n", encoder, incrementer);
#endif

  int enc_ix = (enc_group * mbng_patch_cfg.enc_group_size + encoder) % MBNG_PATCH_NUM_ENC;

  // get encoder configuration from patch structure
  mbng_patch_enc_entry_t *enc_cfg = (mbng_patch_enc_entry_t *)&mbng_patch_enc[enc_ix];

  // TODO: implement different MIDI modes
  // currently we only provide absolute values
  u8 value = 0;
  switch( enc_cfg->mode ) {
  case 0:
    return 0; // encoder disabled

  default: { // absolute mode
    int new_value = enc_value[enc_ix] + incrementer;
    if( new_value < enc_cfg->min )
      new_value = enc_cfg->min;
    else if( new_value > enc_cfg->max )
      new_value = enc_cfg->max;      

    if( new_value == enc_value[enc_ix] )
      return 0; // no change

    enc_value[enc_ix] = new_value;
    value = new_value & 0x7f; // TODO: NRPN etc for higher value range
  }
  }

  // determine MIDI event type
  mios32_midi_event_t event = (enc_cfg->evnt0 >> 4) | 0x8;

  // create MIDI package
  mios32_midi_package_t p;
  p.ALL = 0;
  p.type = event;
  p.event = event;

  if( mbng_patch_cfg.global_chn )
    p.chn = mbng_patch_cfg.global_chn - 1;
  else
    p.chn = enc_cfg->evnt0;

  p.evnt1 = enc_cfg->evnt1;
  p.evnt2 = value;

  // send MIDI package over enabled ports
  int i;
  u16 mask = 1;
  for(i=0; i<16; ++i, mask <<= 1) {
    if( enc_cfg->enabled_ports & mask ) {
      // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
      mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
      MIOS32_MIDI_SendPackage(port, p);
    }
  }

  return 0; // no error
}
