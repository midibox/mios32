// $Id$
//! \defgroup MBNG_AIN
//! AIN access functions for MIDIbox NG
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
#include "mbng_ain.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////

static u16 previous_ain_value[MBNG_PATCH_NUM_AIN];

/////////////////////////////////////////////////////////////////////////////
//! This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_AIN; ++i) {
    previous_ain_value[i] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function handles the various AIN modes
//! Note: it's also used by the AINSER module, therefore public
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_HandleAinMode(mbng_event_item_t *item, u16 value, u16 prev_value, s16 min, s16 max)
{
  mbng_event_ain_mode_t ain_mode = item->flags.AIN.ain_mode; // also valid for AINSER

  switch( ain_mode ) {
  case MBNG_EVENT_AIN_MODE_SNAP: {
    if( value == item->value )
      return -1; // value already sent

    if( item->flags.general.value_from_midi ) {
      int diff = value - prev_value;
      if( diff >= 0 ) { // moved clockwise
	if( prev_value > item->value || value < item->value ) // wrong direction, or target value not reached yet
	  return -2; // don't send
      } else { // moved counter-clockwise
	if( prev_value < item->value || value > item->value ) // wrong direction, target value not reached yet
	  return -2; // don't send
      }
    }
  } break;

  case MBNG_EVENT_AIN_MODE_RELATIVE: {
    int diff = value - prev_value;
    int new_value = item->value + diff;

    if( diff >= 0 ) { // moved clockwise
      if( item->min <= item->max ) {
	if( new_value >= item->max )
	  new_value = item->max;
      } else {
	if( new_value >= item->min )
	  new_value = item->min;
      }
    } else { // moved counter-clockwise
      if( item->min <= item->max ) {
	if( new_value < item->min )
	  new_value = item->min;
      } else {
	if( new_value < item->max )
	  new_value = item->max;
      }
    }
    value = new_value;
  } break;

  case MBNG_EVENT_AIN_MODE_PARALLAX: {
    // see also http://www.ucapps.de/midibox/midibox_plus_parallax.gif
    if( item->flags.general.value_from_midi ) {
      int diff = value - prev_value;
      if( diff >= 0 ) { // moved clockwise
	if( prev_value > item->value || value < item->value ) { // wrong direction, or target value not reached yet
	  if( item->min <= item->max ) {
	    if( (item->max - value) > 0 ) {
	      value = item->value + ((item->max - item->value) / (item->max - value));
	    }
	  } else {
	    if( (item->min - value) > 0 )
	      value = item->value + ((item->min - item->value) / (item->min - value));
	  }
	  return value; // not taken over
	}
      } else { // moved counter-clockwise
	if( prev_value < item->value || value > item->value ) { // wrong direction, target value not reached yet
	  if( value )
	    value = item->value - (item->value / value);
	  return value; // not taken over
	}
      }
    }

    if( value == item->value )
      return -1; // value already sent    
  } break;

  default: // MBNG_EVENT_AIN_MODE_DIRECT:
    if( value == item->value )
      return -1; // value already sent    
  }

  item->value = value; // taken over

  return 0; // value taken over
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called from APP_AIN_NotifyChange when an input
//! has changed its value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_AIN )
    return -1; // invalid pin

  if( !(mbng_patch_ain.enable_mask & (1 << pin)) )
    return 0; // not enabled

  u16 hw_id = MBNG_EVENT_CONTROLLER_AIN + pin + 1;
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyChange(%d, %d)\n", hw_id & 0xfff, pin_value);
  }

  {
    int min = mbng_patch_ain.pin_min_value[pin];
    int max = mbng_patch_ain.pin_max_value[pin];

    if( min != 0 || max != MBNG_PATCH_AIN_MAX_VALUE ) {
      int range = (min <= max) ? (max-min+1) : (min-max+1);

      int value_normalized = pin_value - min;
      if( value_normalized < 0 )
	value_normalized = 0;

      // fixed point arithmetic
      int value_scaled = (MBNG_PATCH_AIN_MAX_VALUE * value_normalized) / range;

      if( value_scaled >= MBNG_PATCH_AIN_MAX_VALUE )
	value_scaled = MBNG_PATCH_AIN_MAX_VALUE - 1;

      if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("MBNG_AIN: pinrange=%d:%d -> scaled %d to %d\n", min, max, pin_value, value_scaled);
      }

      pin_value = value_scaled;
    }
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
	DEBUG_MSG("No event assigned to AIN hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item);
    }

    // scale 12bit value between min/max with fixed point artithmetic
    int value = pin_value;
    s16 min = item.min;
    s16 max = item.max;
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item.map, &map_values);
    if( map_len > 0 ) {
      min = 0;
      max = map_len-1;
    }

    if( min <= max ) {
      value = min + (((256*value)/4096) * (max-min+1) / 256);
    } else {
      value = min - (((256*value)/4096) * (min-max+1) / 256);
    }

    if( map_len > 0 ) {
      value = map_values[value];
    }

    if( pin >= 0 || pin < MBNG_PATCH_NUM_AIN ) {
      int prev_value = previous_ain_value[pin];
      previous_ain_value[pin] = pin_value; // for next notification
      if( min <= max ) {
	prev_value = min + (((256*prev_value)/4096) * (max-min+1) / 256);
      } else {
	prev_value = min - (((256*prev_value)/4096) * (min-max+1) / 256);
      }

      if( map_len > 0 ) {
	prev_value = map_values[prev_value];
      }

      if( MBNG_AIN_HandleAinMode(&item, value, prev_value, min, max) < 0 )
	continue; // don't send
    } else {
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
s32 MBNG_AIN_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}

//! \}
