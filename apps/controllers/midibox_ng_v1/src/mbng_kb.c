// $Id: mbng_din.c 1684 2013-02-06 20:40:38Z tk $
//! \defgroup MBNG_KB
//! Keyboard access functions for MIDIbox NG
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
#include "mbng_kb.h"
#include "mbng_event.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the KB handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by keyboard.c when a key has been toggled
//! It's activated from mios32_config.h:
//! #define KEYBOARD_NOTIFY_TOGGLE_HOOK MBNG_KB_NotifyToggle
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_NotifyToggle(u8 kb, u8 note_number, u8 velocity)
{
  u16 hw_id = MBNG_EVENT_CONTROLLER_KB + kb + 1;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_KB_NotifyToggle(%d, %d, %d)\n", hw_id & 0xfff, note_number, velocity);
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
      MBNG_EVENT_ItemPrint(&item);
    }

    // transpose
    if( item.stream_size >= 2 ) {
      s8 key_transpose = (s8)item.flags.KB.key_transpose;
      int note = note_number + key_transpose;
      if( note < 0 )
	note = 0;
      else if( note > 127 )
	note = 127;

      item.stream[1] = note;
    }

    if( velocity == 0 ) { // we should always send Note-Off on velocity==0
      item.value = 0;
    } else {
      // scale 7bit value between min/max with fixed point artithmetic
      int value = velocity;
      s16 min = item.min;
      s16 max = item.max;
      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
      if( map_len > 0 ) {
	min = 0;
	max = map_len-1;
      }

      if( min <= max ) {
	value = min + (((256*value)/128) * (max-min+1) / 256);
      } else {
	value = min - (((256*value)/128) * (min-max+1) / 256);
      }

      if( map_len > 0 ) {
	value = map_values[value];
      }

      item.value = value;
    }

    if( MBNG_EVENT_NotifySendValue(&item) == 2 )
      break; // stop has been requested
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_KB_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}

//! \}
