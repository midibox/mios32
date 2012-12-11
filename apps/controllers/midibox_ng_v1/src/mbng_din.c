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

static u8 button_group;

static u32 toggle_flags[MBNG_PATCH_NUM_DIN/32];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  button_group = 0;

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

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyToggle(%d, %d)\n", pin, pin_value);
  }

  // search for DIN
  int button_ix = button_group * mbng_patch_cfg.button_group_size + pin;
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(MBNG_EVENT_CONTROLLER_BUTTON + button_ix + 1, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to BUTTON %d\n", button_ix);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // button depressed? (take inverse flag into account)
  // note: on a common configuration (MBHP_DINX4 module used with pull-ups), pins are inverse
  u8 depressed = pin_value ? 0 : 1;
  if( item.flags.DIN.inverse )
    depressed ^= 1;

  // toggle mode?
  if( item.flags.DIN.button_mode == MBNG_EVENT_BUTTON_MODE_TOGGLE ) {
    if( depressed )
      return 0;

    int ix = pin / 32;
    int mask = (1 << (pin % 32));
    toggle_flags[ix] ^= mask;
    depressed = (toggle_flags[ix] & mask) ? 0 : 1;

  } else if( depressed && item.flags.DIN.button_mode == MBNG_EVENT_BUTTON_MODE_ON_ONLY ) {
    return 0; // don't send if button depressed
  }

  u16 value = depressed ? item.min : item.max;

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
s32 MBNG_DIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int button_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyReceivedValue(%d, %d)\n", button_ix, value);
  }

  // forward
  if( item->fwd_id ) {
    u16 item_id_lower = (item->id & 0xfff) - 1;
    if( item_id_lower >= button_group*mbng_patch_cfg.button_group_size &&
	item_id_lower < (button_group+1)*mbng_patch_cfg.button_group_size ) {
      MBNG_EVENT_ItemForward(item, value);
    }
  }

  return 0; // no error
}

