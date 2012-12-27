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


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

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

  // toggle mode?
  if( item.flags.DIN.button_mode == MBNG_EVENT_BUTTON_MODE_TOGGLE ) {
    if( depressed )
      return 0;

    int range = (item.min <= item.max) ? (item.max - item.min + 1) : (item.min - item.max + 1);
    int toggle_state = (item.min <= item.max) ? ((item.value - item.min) >= (range/2)) : ((item.value - item.max) >= (range/2));
    item.value = toggle_state ? item.min : item.max;
  } else {
    item.value = depressed ? item.min : item.max;
  }

  // OnOnly mode: don't send if button depressed
  if( !depressed || item.flags.DIN.button_mode != MBNG_EVENT_BUTTON_MODE_ON_ONLY ) {
    // send MIDI event
    MBNG_EVENT_NotifySendValue(&item);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_NotifyReceivedValue(mbng_event_item_t *item)
{
  int din_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyReceivedValue(%d, %d)\n", din_subid, item->value);
  }

  // nothing else to do...

  return 0; // no error
}
