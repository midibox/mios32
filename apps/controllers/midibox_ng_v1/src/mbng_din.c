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

#include "app.h"
#include "mbng_din.h"
#include "mbng_lcd.h"
#include "mbng_event.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u32 button_states[MBNG_PATCH_NUM_DIN/32];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_DIN/32; ++i)
    button_states[i] = 0;

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

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyToggle(%d, %d)\n", pin, pin_value);
  }

  // get ID
  mbng_event_item_id_t din_id = MBNG_EVENT_CONTROLLER_BUTTON + pin + 1;
  MBNG_PATCH_BankCtrlIdGet(pin, &din_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(din_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to BUTTON id=%d\n", din_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // button depressed?
  u8 depressed = pin_value ? 1 : 0;
  u16 din_subid = din_id & 0xfff;
  int ix = (din_subid-1) / 32;
  int mask = (1 << ((din_subid-1) % 32));
  u16 value = depressed ? item.min : item.max;

  // toggle mode?
  if( item.flags.DIN.button_mode == MBNG_EVENT_BUTTON_MODE_TOGGLE ) {
    if( depressed )
      return 0;

    if( button_states[ix] & mask ) {
      value = item.min;
      button_states[ix] &= ~mask;
    } else {
      value = item.max;
      button_states[ix] |= mask;
    }
  } else {
    if( depressed && !item.flags.DIN.radio_group ) // in radio groups the state can only be cleared by other elements of the group
      button_states[ix] &= ~mask;
    else
      button_states[ix] |= mask;
  }

  // OnOnly mode: don't send if button depressed
  if( !depressed || item.flags.DIN.button_mode != MBNG_EVENT_BUTTON_MODE_ON_ONLY ) {
    // send MIDI event
    MBNG_EVENT_ItemSend(&item, value);

    // forward - optionally to whole radio group
    if( item.flags.DIN.radio_group )
      MBNG_EVENT_ItemForwardToRadioGroup(&item, value, item.flags.DIN.radio_group);
    else
      MBNG_EVENT_ItemForward(&item, value);

    // print label
    MBNG_LCD_PrintItemLabel(&item, value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int din_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyReceivedValue(%d, %d)\n", din_subid, value);
  }

  int range = (item->min <= item->max) ? (item->max - item->min + 1) : (item->min - item->max + 1);
  u8 din_value;
  if( item->flags.DIN.radio_group ) {
    if( item->min <= item->max )
      din_value = value >= item->min && value <= item->max;
    else
      din_value = value >= item->max && value <= item->min;
  } else if( item->min == item->max ) {
    din_value = value == item->min;
  } else
    din_value = (item->min <= item->max) ? ((value - item->min) >= (range/2)) : ((value - item->max) >= (range/2));

  if( din_subid && din_subid <= MBNG_PATCH_NUM_DIN ) {
    int ix = (din_subid-1) / 32;
    int mask = (1 << ((din_subid-1) % 32));

    if( din_value ) {
      button_states[ix] |= mask;
    } else {
      button_states[ix] &= ~mask;
    }
  }  

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the value of a given item ID
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_GetCurrentValueFromId(mbng_event_item_id_t id)
{
  int din_subid = (id & 0xfff);

  if( !din_subid || din_subid > MBNG_PATCH_NUM_DIN )
    return -1; // item not mapped to hardware

  int ix = (din_subid-1) / 32;
  int mask = (1 << ((din_subid-1) % 32));
  return (button_states[ix] & mask) ? 1 : 0;
}
