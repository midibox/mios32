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
#include <ainser.h>

#include "app.h"
#include "mbng_ain.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u16 ain_value[MBNG_PATCH_NUM_AIN];
static u16 previous_ain_value[MBNG_PATCH_NUM_AIN];
static u16 ainser_value[MBNG_PATCH_NUM_AINSER_MODULES*64];

/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_AIN; ++i) {
    ain_value[i] = 0;
    previous_ain_value[i] = 0;
  }

  for(i=0; i<MBNG_PATCH_NUM_AINSER_MODULES*64; ++i)
    ainser_value[i] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function handles the various AIN modes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_AIN_HandleAinMode(mbng_event_ain_mode_t ain_mode, u16 value, u16 prev_value, u16 stored_value, s16 min_value, s16 max_value)
{
  u8 taken_over = stored_value >= 0x8000;
  stored_value &= 0x7fff;

  switch( ain_mode ) {
  case MBNG_EVENT_AIN_MODE_SNAP: {
    if( value == stored_value )
      return -1; // value already sent

    if( !taken_over ) {
      int diff = value - prev_value;
      if( diff >= 0 ) { // moved clockwise
	if( prev_value > stored_value || value < stored_value ) // wrong direction, or target value not reached yet
	  return -2; // don't send
      } else { // moved counter-clockwise
	if( prev_value < stored_value || value > stored_value ) // wrong direction, target value not reached yet
	  return -2; // don't send
      }
    }
  } break;

  case MBNG_EVENT_AIN_MODE_RELATIVE: {
    int diff = value - prev_value;
    int new_value = stored_value + diff;

    if( diff >= 0 ) { // moved clockwise
      if( min_value <= max_value ) {
	if( new_value >= max_value )
	  new_value = max_value;
      } else {
	if( new_value >= min_value )
	  new_value = min_value;
      }
    } else { // moved counter-clockwise
      if( min_value <= max_value ) {
	if( new_value < min_value )
	  new_value = min_value;
      } else {
	if( new_value < max_value )
	  new_value = max_value;
      }
    }
    value = new_value;
  } break;

  case MBNG_EVENT_AIN_MODE_PARALLAX: {
    // see also http://www.ucapps.de/midibox/midibox_plus_parallax.gif
    if( !taken_over ) {
      int diff = value - prev_value;
      if( diff >= 0 ) { // moved clockwise
	if( prev_value > stored_value || value < stored_value ) { // wrong direction, or target value not reached yet
	  if( min_value <= max_value ) {
	    if( (max_value - value) > 0 ) {
	      value = stored_value + ((max_value - stored_value) / (max_value - value));
	    }
	  } else {
	    if( (min_value - value) > 0 )
	      value = stored_value + ((min_value - stored_value) / (min_value - value));
	  }
	  return value; // not taken over
	}
      } else { // moved counter-clockwise
	if( prev_value < stored_value || value > stored_value ) { // wrong direction, target value not reached yet
	  if( value )
	    value = stored_value - (stored_value / value);
	  return value; // not taken over
	}
      }
    }

    if( value == stored_value )
      return -1; // value already sent    
  } break;

  default: // MBNG_EVENT_AIN_MODE_DIRECT:
    if( value == stored_value )
      return -1; // value already sent    
  }

  return value | 0x8000; // taken over
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_AIN_NotifyChange when an input
// has changed its value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_AIN )
    return -1; // invalid pin

  if( !(mbng_patch_ain.enable_mask & (1 << pin)) )
    return 0; // not enabled

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyChange(%d, %d)\n", pin, pin_value);
  }

  // get ID
  mbng_event_item_id_t ain_id = MBNG_EVENT_CONTROLLER_AIN + pin + 1;
  MBNG_PATCH_BankCtrlIdGet(pin, &ain_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(ain_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AIN id=%d\n", ain_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value;
  if( item.min <= item.max ) {
    value = item.min + (((256*pin_value)/4096) * (item.max-item.min+1) / 256);
  } else {
    value = item.min - (((256*pin_value)/4096) * (item.min-item.max+1) / 256);
  }

  int ain_ix = (ain_id & 0xfff) - 1;
  if( ain_ix >= 0 || ain_ix < MBNG_PATCH_NUM_AIN ) {
    int prev_value = previous_ain_value[ain_ix];
    previous_ain_value[ain_ix] = pin_value; // for next notification
    if( item.min <= item.max ) {
      prev_value = item.min + (((256*prev_value)/4096) * (item.max-item.min+1) / 256);
    } else {
      prev_value = item.min - (((256*prev_value)/4096) * (item.min-item.max+1) / 256);
    }

    int new_value;
    if( (new_value=MBNG_AIN_HandleAinMode(item.flags.AIN.ain_mode, value, prev_value, ain_value[ain_ix], item.min, item.max)) < 0 )
      return 0; // don't send

    if( new_value & 0x8000 )
      ain_value[ain_ix] = new_value;
    value = new_value & 0x7fff;
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
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value)
{
  // actual module number based on CS config
  u8 mapped_module = 0;
  {
    int i;
    mbng_patch_ainser_entry_t *ainser = (mbng_patch_ainser_entry_t *)&mbng_patch_ainser[0];
    for(i=0; i<MBNG_PATCH_NUM_AINSER_MODULES; ++i, ++ainser) {
      if( ainser->flags.enabled && ainser->flags.cs == module )
	break;
    }

    if( i >= MBNG_PATCH_NUM_AINSER_MODULES )
      return -2; // module not mapped

    mapped_module = i;
  }

  int mbng_pin = mapped_module*64 + pin;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyChange(%d, %d, %d)\n", mapped_module, mbng_pin, pin_value);
  }

  // get ID
  mbng_event_item_id_t ainser_id = MBNG_EVENT_CONTROLLER_AINSER + mbng_pin + 1;
  MBNG_PATCH_BankCtrlIdGet(mbng_pin, &ainser_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(ainser_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AINSER id=%d\n", ainser_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = pin_value;
  if( item.min <= item.max ) {
    value = item.min + (((256*value)/4096) * (item.max-item.min+1) / 256);
  } else {
    value = item.min - (((256*value)/4096) * (item.min-item.max+1) / 256);
  }

  int prev_value = AINSER_PreviousPinValueGet();
  if( item.min <= item.max ) {
    prev_value = item.min + (((256*prev_value)/4096) * (item.max-item.min+1) / 256);
  } else {
    prev_value = item.min - (((256*prev_value)/4096) * (item.min-item.max+1) / 256);
  }

  int ainser_ix = (ainser_id & 0xfff) - 1;
  if( ainser_ix >= 0 || ainser_ix < MBNG_PATCH_NUM_AIN ) {
    int new_value;
    if( (new_value=MBNG_AIN_HandleAinMode(item.flags.AINSER.ain_mode, value, prev_value, ainser_value[ainser_ix], item.min, item.max)) < 0 )
      return 0; // don't send

    if( new_value & 0x8000 )
      ainser_value[ainser_ix] = new_value;
    value = new_value & 0x7fff;
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
s32 MBNG_AIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int ain_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyReceivedValue(%d, %d)\n", ain_subid, value);
  }

  // store new value
  if( ain_subid && ain_subid <= MBNG_PATCH_NUM_AIN )
    ain_value[ain_subid-1] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the value of a given item ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_GetCurrentValueFromId(mbng_event_item_id_t id)
{
  int ain_subid = id & 0xfff;

  if( !ain_subid || ain_subid > MBNG_PATCH_NUM_AIN )
    return -1; // item not mapped to hardware

  return ain_value[ain_subid-1];
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int ainser_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyReceivedValue(%d, %d)\n", ainser_subid, value);
  }

  // store new value
  if( ainser_subid && ainser_subid <= MBNG_PATCH_NUM_AINSER_MODULES*64 )
    ainser_value[ainser_subid-1] = value;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the value of a given item ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_GetCurrentValueFromId(mbng_event_item_id_t id)
{
  int ainser_subid = id & 0xfff;

  if( !ainser_subid || ainser_subid > MBNG_PATCH_NUM_AINSER_MODULES*64 )
    return -1; // item not mapped to hardware

  return ainser_value[ainser_subid-1];
}
