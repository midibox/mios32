// $Id$
/*
 * Motorfader access functions for MIDIbox NG
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

#include "app.h"
#include "tasks.h"
#include "mbng_mf.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u16 mf_value[MBNG_PATCH_NUM_MF_MODULES*8];
static u8  mf_value_msb[MBNG_PATCH_NUM_MF_MODULES][8];
static u8  mf_touchsensor_selected[MBNG_PATCH_NUM_MF_MODULES];

/////////////////////////////////////////////////////////////////////////////
// This function initializes the MF handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MF_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_MF_MODULES*8; ++i)
    mf_value[i] = 0;

  for(i=0; i<MBNG_PATCH_NUM_MF_MODULES; ++i) {
    int j;
    for(j=0; j<8; ++j)
      mf_value_msb[i][j] = 0;
    mf_touchsensor_selected[i] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_MIDI_NotifyPackage whenver a new
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MF_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // motorfaders will send/receive the Motormix protocol!

  if( midi_package.event != CC )
    return 0; // not for interest

  u8 is_fader_msb       = (midi_package.cc_number & 0xf8) == 0x00;
  u8 is_fader_lsb       = (midi_package.cc_number & 0xf8) == 0x20;
  u8 is_touchsensor_sel = midi_package.cc_number == 0x0f;
  u8 is_touchsensor_val = midi_package.cc_number == 0x2f;

  int module;
  mbng_patch_mf_entry_t *mf = (mbng_patch_mf_entry_t *)&mbng_patch_mf[0];
  for(module=0; module<MBNG_PATCH_NUM_MF_MODULES; ++module, ++mf) {
    if( mf->flags.enabled && midi_package.chn == mf->flags.chn &&
	(port == mf->midi_in_port || port == mf->config_port) ) {

      // forward to config or MIDI OUT port
      mios32_midi_port_t out_port = (port == mf->midi_in_port) ? mf->config_port : mf->midi_out_port;
      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendPackage(out_port, midi_package);
      MUTEX_MIDIOUT_GIVE;

      if( is_touchsensor_sel ) {
	mf_touchsensor_selected[module] = midi_package.value;
      } else if( is_touchsensor_val ) {
	u8 is_pressed = midi_package.value >= 0x40;
	u8 ts_sel = mf_touchsensor_selected[module];
	if( ts_sel < 8 && mf->ts_first_button_id ) {

	  // get ID
	  mbng_event_item_id_t button_id = mf->ts_first_button_id + ts_sel;
	  mbng_event_item_t item;
	  if( MBNG_EVENT_ItemSearchById(button_id, &item) < 0 ) {
	    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	      DEBUG_MSG("No event assigned to BUTTON id=%d for touchsensor\n", button_id & 0xfff);
	    }
	    return -2; // no event assigned
	  }

	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    MBNG_EVENT_ItemPrint(&item);
	  }

	  u16 value = is_pressed ? item.max : item.min;

	  // send MIDI event
	  MBNG_EVENT_ItemSend(&item, value);

	  // forward
	  MBNG_EVENT_ItemForward(&item, value);

	  // print label
	  MBNG_LCD_PrintItemLabel(&item, value);
	}
      } else if( is_fader_msb ) {
	u8 fader_sel = midi_package.cc_number & 0x7;
	mf_value_msb[module][fader_sel] = midi_package.value;
      } else if( is_fader_lsb ) {
	u8 fader_sel = midi_package.cc_number & 0x7;
	u16 value = midi_package.value | ((u16)mf_value_msb[module][fader_sel] << 7);
	u16 fader = module*8 + fader_sel;

	// get ID
	mbng_event_item_id_t mf_id = MBNG_EVENT_CONTROLLER_MF + fader + 1;
	MBNG_PATCH_BankCtrlIdGet(fader, &mf_id); // modifies id depending on bank selection
	mbng_event_item_t item;
	if( MBNG_EVENT_ItemSearchById(mf_id, &item) < 0 ) {
	  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	    DEBUG_MSG("No event assigned to MF id=%d\n", mf_id & 0xfff);
	  }
	  return -2; // no event assigned
	}

	if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	  MBNG_EVENT_ItemPrint(&item);
	}

	// scale value from 14bit
	int range = item.max - item.min + 1;
	if( range < 0 ) range *= -1;
	u32 value_scaled = value / (16384 / range);

	u8 value_changed = 1;
	int mf_ix = (mf_id & 0xfff) - 1;
	if( mf_ix >= 0 || mf_ix < MBNG_PATCH_NUM_MF_MODULES*8 ) {
	  if( mf_value[mf_ix] == value_scaled ) {
	    value_changed = 0;
	  } else {
	    mf_value[mf_ix] = value_scaled;
	  }
	}

	if( value_changed ) {
	  // send MIDI event
	  MBNG_EVENT_ItemSend(&item, value_scaled);

	  // forward
	  MBNG_EVENT_ItemForward(&item, value_scaled);

	  // print label
	  MBNG_LCD_PrintItemLabel(&item, value_scaled);
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_SYSEX_Parser on incoming SysEx data
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MF_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  int module;
  mbng_patch_mf_entry_t *mf = (mbng_patch_mf_entry_t *)&mbng_patch_mf[0];
  for(module=0; module<MBNG_PATCH_NUM_MF_MODULES; ++module, ++mf) {
    if( mf->flags.enabled &&
	(port == mf->midi_in_port || port == mf->config_port) ) {

      mios32_midi_port_t out_port = (port == mf->midi_in_port) ? mf->config_port : mf->midi_out_port;

      // forward as single byte
      // TODO: optimize this by collecting up to 3 data words and put it into package
      MUTEX_MIDIOUT_TAKE;
      mios32_midi_package_t midi_package;
      midi_package.ALL = 0x00000000;
      midi_package.type = 0xf; // single byte
      midi_package.evnt0 = midi_in;
      MIOS32_MIDI_SendPackage(out_port, midi_package);
      MUTEX_MIDIOUT_GIVE;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MF_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int mf_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MF_NotifyReceivedValue(%d, %d)\n", mf_subid, value);
  }

  // store new value and forward to MF
  if( mf_subid && mf_subid <= MBNG_PATCH_NUM_MF_MODULES*8 ) {
    mf_value[mf_subid-1] = value;

    int module = (mf_subid-1) / 8;
    int fader = (mf_subid-1) % 8;
    mbng_patch_mf_entry_t *mf = (mbng_patch_mf_entry_t *)&mbng_patch_mf[module];
    if( mf->flags.enabled ) {
      // scale value to 14bit
      int range = item->max - item->min + 1;
      if( range < 0 ) range *= -1;
      u32 value14 = value * (16384 / range);

      MUTEX_MIDIOUT_TAKE;
      MIOS32_MIDI_SendCC(mf->midi_out_port, mf->flags.chn, 0x00 + fader, (value14 >> 7) & 0x7f);
      MIOS32_MIDI_SendCC(mf->midi_out_port, mf->flags.chn, 0x20 + fader, value14 & 0x7f);
      MUTEX_MIDIOUT_GIVE;    
    }
  }

  // forward
  if( item->fwd_id && (!MBNG_PATCH_BankCtrlInBank(item) || MBNG_PATCH_BankCtrlIsActive(item)) )
    MBNG_EVENT_ItemForward(item, value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_Refresh() to refresh the controller
// (mainly to trigger the forward item)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_MF_NotifyRefresh(mbng_event_item_t *item)
{
  int mf_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_MF_NotifyRefresh(%d)\n", mf_subid);
  }

  if( mf_subid && mf_subid <= MBNG_PATCH_NUM_MF_MODULES*8 ) {
    u16 value = mf_value[mf_subid-1];
    MBNG_MF_NotifyReceivedValue(item, value);

    // print label
    MBNG_LCD_PrintItemLabel(item, value);
  }

  return 0; // no error
}
