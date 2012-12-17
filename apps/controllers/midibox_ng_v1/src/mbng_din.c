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

    value = (button_states[ix] & mask) ? item.min : item.max;
  }

  if( value == item.max )
    button_states[ix] |= mask;
  else
    button_states[ix] &= ~mask;

  // OnOnly mode: don't send if button depressed
  if( !depressed || item.flags.DIN.button_mode != MBNG_EVENT_BUTTON_MODE_ON_ONLY ) {
    // send MIDI event
    MBNG_EVENT_ItemSend(&item, value);

    // forward
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

  int range = item->max - item->min + 1;
  if( range < 0 ) range *= -1;
  u8 din_value = value >= (range/2);

  if( din_subid && din_subid <= MBNG_PATCH_NUM_DIN ) {
    int ix = (din_subid-1) / 32;
    int mask = (1 << ((din_subid-1) % 32));

    if( din_value ) {
      button_states[ix] |= mask;
    } else {
      button_states[ix] &= ~mask;
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
s32 MBNG_DIN_NotifyRefresh(mbng_event_item_t *item)
{
  int din_subid = (item->id & 0xfff);

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyRefresh(%d)\n", din_subid);
  }

  if( din_subid && din_subid <= MBNG_PATCH_NUM_DIN ) {
    int ix = (din_subid-1) / 32;
    int mask = (1 << ((din_subid-1) % 32));
    u16 value = (button_states[ix] & mask) ? item->max : item->min;

    MBNG_DIN_NotifyReceivedValue(item, value);

    // print label
    MBNG_LCD_PrintItemLabel(item, value);
  }

  return 0; // no error
}
