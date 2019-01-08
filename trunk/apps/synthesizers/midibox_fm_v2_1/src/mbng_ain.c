// $Id: mbng_ain.c 2019 2014-06-04 19:09:46Z tk $
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
static u32 ain_timeout[MBNG_PATCH_NUM_AIN];

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
    ain_timeout[i] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function handles the calibration values
//! Note: it's also used by the AINSER module, therefore public
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_HandleCalibration(u16 pin_value, u16 min, u16 max, u16 ain_max, u8 spread_center)
{
  if( min != 0 || max != ain_max || spread_center ) {
    int range = max - min + 1;

    if( range < 0 )
      return pin_value;

    int value_normalized = pin_value - min;
    if( value_normalized < 0 )
      value_normalized = 0;

    int value_scaled;
    int centered = 0;
    if( spread_center ) {
      int middle_band = 80; // +/- 40
      int middle = range / 2;

      if( value_normalized < (middle-(middle_band/2)) ) {
	value_scaled = (ain_max * value_normalized) / (range-middle_band);
      } else if( value_normalized < (middle+(middle_band/2)) ) {
	value_scaled = (ain_max+1) / 2;
	centered = 1;
      } else {
	value_scaled = (ain_max * (value_normalized-middle_band)) / (range-middle_band);
      }
    } else {
      // fixed point arithmetic
      value_scaled = (ain_max * value_normalized) / range;
    }

    if( value_scaled > ain_max )
      value_scaled = ain_max;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("analog pinrange=%d:%d -> scaled %d to %d%s\n", min, max, pin_value, value_scaled, centered ? " (centered)" : "");
    }

    pin_value = value_scaled;
  }

  return pin_value;
}


/////////////////////////////////////////////////////////////////////////////
//! This function handles the various AIN modes
//! Note: it's also used by the AINSER module, therefore public
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_HandleAinMode(mbng_event_item_t *item, u16 pin_value, u16 prev_pin_value)
{
  mbng_event_ain_mode_t ain_mode = item->custom_flags.AIN.ain_mode; // also valid for AINSER

  // handle switch mode
  if( ain_mode == MBNG_EVENT_AIN_MODE_SWITCH ) {
    // hysteresis:
    u32 threshold_low  = (0x1000 * 30) / 100;
    u32 threshold_high = (0x1000 * 70) / 100;

    if( prev_pin_value >= threshold_low && pin_value < threshold_low ) {
      pin_value = 0x000;
      prev_pin_value = 0xfff;
    } else if( prev_pin_value < threshold_high && pin_value >= threshold_high ) {
      pin_value = 0xfff;
      prev_pin_value = 0x000;
    } else {
      return -2; // don't send
    }
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = pin_value;
  s16 min = item->min;
  s16 max = item->max;
  u8 *map_values;
  int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
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

  int prev_value;
  if( min <= max ) {
    prev_value = min + (((256*prev_pin_value)/4096) * (max-min+1) / 256);
  } else {
    prev_value = min - (((256*prev_pin_value)/4096) * (min-max+1) / 256);
  }

  if( map_len > 0 ) {
    prev_value = map_values[prev_value];
  }

  switch( ain_mode ) {
  case MBNG_EVENT_AIN_MODE_SNAP: {
    if( value == item->value )
      return -1; // value already sent

    if( item->flags.value_from_midi ) {
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
    if( item->flags.value_from_midi ) {
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

  case MBNG_EVENT_AIN_MODE_SWITCH: // no additional handling required (hysteresis was handled at the begin of this function)
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
s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value, u8 no_midi)
{
  if( pin >= MBNG_PATCH_NUM_AIN )
    return -1; // invalid pin

  if( !(mbng_patch_ain.enable_mask & (1 << pin)) )
    return 0; // not enabled

  u16 hw_id = MBNG_EVENT_CONTROLLER_AIN + pin + 1;
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyChange(%d, %d)\n", hw_id & 0xfff, pin_value);
  }

  // Optional Calibration
  u8 is_pin_value_min = pin_value <= mbng_patch_ain.cali[pin].min;
  pin_value = MBNG_AIN_HandleCalibration(pin_value,
					 mbng_patch_ain.cali[pin].min,
					 mbng_patch_ain.cali[pin].max,
					 MBNG_PATCH_AIN_MAX_VALUE,
					 mbng_patch_ain.cali[pin].spread_center);

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( !no_midi && debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to AIN hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }


    //u16 prev_item_value = item.value;
    if( pin >= 0 || pin < MBNG_PATCH_NUM_AIN ) {
      int prev_pin_value = previous_ain_value[pin];
      previous_ain_value[pin] = pin_value; // for next notification

      if( !no_midi && item.custom_flags.AIN.ain_filter_delay_ms ) {
	if( is_pin_value_min ) {
	  ain_timeout[pin] = 0;
	} else {
	  u32 timestamp = MIOS32_TIMESTAMP_Get();
	  u32 timeout = ain_timeout[pin];

	  if( !timeout || timestamp < timeout ) {
	    ain_timeout[pin] = timestamp + item.custom_flags.AIN.ain_filter_delay_ms;
	    continue; // don't send
	  } else {
	    ain_timeout[pin] = 0;
	  }
	}
      }

      if( MBNG_AIN_HandleAinMode(&item, pin_value, prev_pin_value) < 0 )
	continue; // don't send
    }

    if( no_midi ) {
      MBNG_EVENT_ItemCopyValueToPool(&item);
    } else {
      if( MBNG_EVENT_NotifySendValue(&item) == 2 )
	break; // stop has been requested
    }
  } while( continue_ix );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Handle timed AIN events (sensor mode)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Periodic(void)
{
  u32 pin;
  u32 *timeout_ptr = (u32 *)&ain_timeout[0];
  u32 timestamp = MIOS32_TIMESTAMP_Get();

  for(pin=0; pin<MBNG_PATCH_NUM_AIN; ++pin, ++timeout_ptr) {
    if( *timeout_ptr && timestamp >= *timeout_ptr ) {
      MBNG_AIN_NotifyChange(pin, MIOS32_AIN_PinGet(pin), 0); // no_midi==0
    }
  }

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
