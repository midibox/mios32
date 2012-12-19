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

#include "app.h"
#include "mbng_enc.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u16 enc_value[MBNG_PATCH_NUM_ENC];

static u8 enc_speed_multiplier;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the ENC handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  enc_speed_multiplier = 0;

  int i;
  for(i=0; i<MBNG_PATCH_NUM_ENC; ++i)
    enc_value[i] = 0;

  for(i=1; i<MIOS32_ENC_NUM_MAX; ++i) { // start at 1 since the first encoder is allocated by SCS
    mios32_enc_config_t enc_config;
    enc_config = MIOS32_ENC_ConfigGet(i);
    enc_config.cfg.sr = 0;
    enc_config.cfg.pos = 0;
    enc_config.cfg.speed = NORMAL;
    enc_config.cfg.speed_par = 0;
    MIOS32_ENC_ConfigSet(i, enc_config);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Enables/Disables fast mode with given multiplier
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_FastModeSet(u8 multiplier)
{
  enc_speed_multiplier = multiplier;
  return 0;
}

s32 MBNG_ENC_FastModeGet(void)
{
  return enc_speed_multiplier;
}


/////////////////////////////////////////////////////////////////////////////
// Sets the encoder speed depending on value range
// should this be optional?
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_AutoSpeed(u32 enc, mbng_event_item_t *item)
{
  int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);

  mios32_enc_config_t enc_config;
  enc_config = MIOS32_ENC_ConfigGet(enc+1); // add +1 since the first encoder is allocated by SCS

  mios32_enc_speed_t cfg_speed = NORMAL;
  int                cfg_speed_par = 0;

  if(        item->flags.ENC.enc_speed_mode == MBNG_EVENT_ENC_SPEED_MODE_SLOW ) {
    cfg_speed = SLOW;
    cfg_speed_par = item->flags.ENC.enc_speed_mode_par;
  } else if( item->flags.ENC.enc_speed_mode <= MBNG_EVENT_ENC_SPEED_MODE_NORMAL ) {
    cfg_speed = NORMAL;
    cfg_speed_par = item->flags.ENC.enc_speed_mode_par;
  } else if( item->flags.ENC.enc_speed_mode <= MBNG_EVENT_ENC_SPEED_MODE_FAST ) {
    cfg_speed = FAST;
    cfg_speed_par = item->flags.ENC.enc_speed_mode_par;
  } else { // MBNG_EVENT_ENC_SPEED_MODE_AUTO
    if( enc_config.cfg.type == NON_DETENTED ) {
      if( range < 32  ) {
	cfg_speed = SLOW;
	cfg_speed_par = 3;
      } else if( range <= 256 ) {
	cfg_speed = NORMAL;
      } else {
	cfg_speed = FAST;
	cfg_speed_par = 2 + (range / 2048);
	if( cfg_speed_par > 7 )
	  cfg_speed_par = 7;
      }
    } else {
      if( range < 32  ) {
	cfg_speed = SLOW;
	cfg_speed_par = 3;
      } else if( range <= 64 ) {
	cfg_speed = NORMAL;
      } else {
	cfg_speed = FAST;
	cfg_speed_par = 2 + (range / 2048);
	if( cfg_speed_par > 7 )
	  cfg_speed_par = 7;
      }
    }
  }

  DEBUG_MSG("%d: %d %d\n", item->id & 0xfff, cfg_speed, cfg_speed_par);
  if( enc_config.cfg.speed != cfg_speed || enc_config.cfg.speed_par != cfg_speed_par ) {
    enc_config.cfg.speed = cfg_speed;
    enc_config.cfg.speed_par = cfg_speed_par;
    MIOS32_ENC_ConfigSet(enc+1, enc_config); // add +1 since the first encoder is allocated by SCS
  }

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

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_ENC_NotifyChange(%d, %d)\n", encoder, incrementer);
  }

  // get ID
  mbng_event_item_id_t enc_id = MBNG_EVENT_CONTROLLER_ENC + encoder + 1;
  MBNG_PATCH_BankCtrlIdGet(encoder, &enc_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(enc_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to ENC id=%d\n", enc_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  if( enc_speed_multiplier > 1 ) {
    incrementer *= (s32)enc_speed_multiplier;
  }

  int value = 0;
  switch( item.flags.ENC.enc_mode ) {
  case MBNG_EVENT_ENC_MODE_40SPEED:
    value = 0x40 + incrementer;
    if( value < 0 )
      value = 0;
    else if( value >= 0x7f )
      value = 0x7f;
    break;

  case MBNG_EVENT_ENC_MODE_00SPEED:
    value = incrementer & 0x7f;
    break;

  case MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED:
    if( incrementer < 0 ) {
      if( incrementer < -63 )
	incrementer = -63;
      value = -incrementer | 0x40;
    } else {
      if( incrementer > 63 )
	incrementer = 63;
      value = incrementer;
    }
    break;

  case MBNG_EVENT_ENC_MODE_INC41_DEC3F:
    value = incrementer > 0 ? 0x41 : 0x3f;
    break;

  case MBNG_EVENT_ENC_MODE_INC01_DEC7F:
    value = incrementer > 0 ? 0x01 : 0x7f;
    break;

  case MBNG_EVENT_ENC_MODE_INC01_DEC41:
    value = incrementer > 0 ? 0x01 : 0x41;
    break;

  case MBNG_EVENT_ENC_MODE_INC_DEC:
    // TODO
    // dummy:
    value = incrementer > 0 ? 0x41 : 0x3f;
    break;

  default: // MBNG_EVENT_ENC_MODE_ABSOLUTE
    MBNG_ENC_AutoSpeed(encoder, &item);

    int enc_ix = (enc_id & 0xfff) - 1;
    if( enc_ix < 0 || enc_ix > MBNG_PATCH_NUM_ENC )
      return 0; // no value storage

    if( item.min <= item.max ) {
      value = enc_value[enc_ix] + incrementer;
      if( value < item.min )
	value = item.min;
      else if( value > item.max )
	value = item.max;
    } else {
      // reversed range
      value = enc_value[enc_ix] - incrementer;
      if( value < item.max )
	value = item.max;
      else if( value > item.min )
	value = item.min;
    }

    if( value == enc_value[enc_ix] )
      return 0; // no change

    enc_value[enc_ix] = value;
  }

  // send MIDI event
  MBNG_EVENT_ItemSend(&item, value);

  // forward
  MBNG_EVENT_ItemForward(&item, value);

  // print label
  MBNG_LCD_PrintItemLabel(&item, value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int enc_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_ENC_NotifyReceivedValue(%d, %d)\n", enc_subid, value);
  }

  // store new value
  if( enc_subid && enc_subid <= MBNG_PATCH_NUM_ENC )
    enc_value[enc_subid-1] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the value of a given item ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_ENC_GetCurrentValueFromId(mbng_event_item_id_t id)
{
  int enc_subid = id & 0xfff;

  if( !enc_subid || enc_subid > MBNG_PATCH_NUM_ENC )
    return -1; // item not mapped to hardware

  return enc_value[enc_subid-1];
}
