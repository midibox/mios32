// $Id$
//! \defgroup MBNG_DIN
//! DIN access functions for MIDIbox NG
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include "app.h"
#include "mbng_din.h"
#include "mbng_lcd.h"
#include "mbng_event.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the DIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_DIN_NotifyToggle when an input
//! has been toggled
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
  u16 hw_id = MBNG_EVENT_CONTROLLER_BUTTON + pin + 1;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyToggle(%d, %d)\n", hw_id & 0xfff, pin_value);
  }

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to BUTTON hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    // button depressed?
    u8 depressed = pin_value ? 1 : 0;

    // toggle mode?
    if( item.flags.DIN.button_mode == MBNG_EVENT_BUTTON_MODE_TOGGLE ) {
      if( depressed )
	return 0;

      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
      if( map_len > 0 ) {
	int map_ix = MBNG_EVENT_MapIxGet(map_values, map_len, item.value) + 1;
	if( map_ix >= map_len )
	  map_ix = 0;
	item.value = map_values[map_ix];
      } else {
	int range = (item.min <= item.max) ? (item.max - item.min + 1) : (item.min - item.max + 1);
	int toggle_state = (item.min <= item.max) ? ((item.value - item.min) >= (range/2)) : ((item.value - item.max) >= (range/2));
	item.value = toggle_state ? item.min : item.max;
      }
    } else {
      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
      if( map_len > 0 ) {
	item.value = depressed ? map_values[0] : map_values[map_len-1];
      } else {
	item.value = depressed ? item.min : item.max;
      }
    }

    // OnOnly mode: don't send if button depressed
    if( !depressed || item.flags.DIN.button_mode != MBNG_EVENT_BUTTON_MODE_ON_ONLY ) {
      if( MBNG_EVENT_NotifySendValue(&item) == 2 )
	break; // stop has been requested
    }
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_DIN_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_DIN_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}

//! \}
