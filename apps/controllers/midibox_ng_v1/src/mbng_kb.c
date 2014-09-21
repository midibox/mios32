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

#include <keyboard.h>
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

  {
    KEYBOARD_Init(0);

    // disable keyboard SR assignments by default
    int kb;
    keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
    for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
      kc->num_rows = 0;
      kc->dout_sr1 = 0;
      kc->dout_sr2 = 0;
      kc->din_sr1 = 0;
      kc->din_sr2 = 0;

      // due to slower scan rate:
      kc->delay_fastest = 5;
      kc->delay_fastest_black_keys = 0; // if 0, we take delay_fastest, otherwise we take the dedicated value for the black keys
      kc->delay_slowest = 100;
    }

    KEYBOARD_Init(1); // re-init runtime variables, don't touch configuration
  }

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
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    // transpose
    s8 kb_transpose = (s8)item.custom_flags.KB.kb_transpose;
    int note = note_number + kb_transpose;
    if( item.stream_size >= 2 ) {
      if( note < 0 )
	note = 0;
      else if( note > 127 )
	note = 127;
    }

    // velocity map
    if( item.custom_flags.KB.kb_velocity_map ) {
      u8 *map_values;
      int map_len = MBNG_EVENT_MapGet(item.custom_flags.KB.kb_velocity_map, &map_values);
      if( map_len > 0 ) {
	velocity = map_values[(velocity > map_len) ? (map_len-1) : velocity];
      }
    }

    if( item.flags.use_key_or_cc ) {
      // for keyboard zones
      if( note_number < item.min || note_number > item.max )
	continue;

      item.value = note;
      item.secondary_value = velocity;
    } else {
      item.stream[1] = note;

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


/////////////////////////////////////////////////////////////////////////////
//! Sets the "break_is_make" switch
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_KB_BreakIsMakeSet(u8 kb, u8 value)
{
  if( kb >= KEYBOARD_NUM )
    return -1; // invalid keyboard

  keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[kb];
  kc->break_is_make = value ? 1 : 0;

  return 0; // no error
}

//! \}
