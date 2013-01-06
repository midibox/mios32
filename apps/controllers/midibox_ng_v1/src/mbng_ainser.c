// $Id$
/*
 * AINSER access functions for MIDIbox NG
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
#include "mbng_ainser.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

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
      if( ainser->flags.cs == module && (AINSER_EnabledGet(module) > 0) )
	break;
    }

    if( i >= MBNG_PATCH_NUM_AINSER_MODULES )
      return -2; // module not mapped

    mapped_module = i;
  }

  int mbng_pin = mapped_module*64 + pin;

  u16 hw_id = MBNG_EVENT_CONTROLLER_AINSER + mbng_pin + 1;
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyChange(%d, %d)\n", hw_id & 0xfff, pin_value);
  }

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchByHwId(hw_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AINSER hw_id=%d\n", hw_id & 0xfff);
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

  int prev_value = AINSER_PreviousPinValueGet();
  if( min <= max ) {
    prev_value = min + (((256*prev_value)/4096) * (max-min+1) / 256);
  } else {
    prev_value = min - (((256*prev_value)/4096) * (min-max+1) / 256);
  }

  if( map_len > 0 ) {
    prev_value = map_values[prev_value];
  }

  if( MBNG_AIN_HandleAinMode(&item, value, prev_value, min, max) < 0 )
    return 0; // don't send

  // send MIDI event
  MBNG_EVENT_NotifySendValue(&item);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_NotifyReceivedValue(mbng_event_item_t *item)
{
  u16 hw_id = item->hw_id;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyReceivedValue(%d, %d)\n", hw_id & 0xfff, item->value);
  }

  // nothing else to do...

  return 0; // no error
}
