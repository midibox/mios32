// $Id$
//! \defgroup MBNG_AINSER
//! AINSER access functions for MIDIbox NG
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
#include <ainser.h>

#include "app.h"
#include "mbng_ain.h"
#include "mbng_ainser.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
//! local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AINSER_NotifyChange(u32 module, u32 pin, u32 pin_value, u8 no_midi)
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

  if( mbng_pin < (MBNG_PATCH_NUM_AINSER_MODULES*AINSER_NUM_PINS) ) {
    // Optional Calibration
    pin_value = MBNG_AIN_HandleCalibration(pin_value,
					   mbng_patch_ainser[mapped_module].cali[pin].min,
					   mbng_patch_ainser[mapped_module].cali[pin].max,
					   MBNG_PATCH_AINSER_MAX_VALUE,
					   mbng_patch_ainser[mapped_module].cali[pin].spread_center);
  }

  // MIDI Learn
  MBNG_EVENT_MidiLearnIt(hw_id);

  // get ID
  mbng_event_item_t item;
  u32 continue_ix = 0;
  do {
    if( MBNG_EVENT_ItemSearchByHwId(hw_id, 0, &item, &continue_ix) < 0 ) {
      if( continue_ix )
	return 0; // ok: at least one event was assigned
      if( !no_midi && debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
	DEBUG_MSG("No event assigned to AINSER hw_id=%d\n", hw_id & 0xfff);
      }
      return -2; // no event assigned
    }

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      MBNG_EVENT_ItemPrint(&item, 0);
    }

    u16 prev_pin_value = AINSER_PreviousPinValueGet();
    if( MBNG_AIN_HandleAinMode(&item, pin_value, prev_pin_value) < 0 )
      continue; // don't send

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
//! This function is called by MBNG_EVENT_ItemReceive when a matching value
//! has been received
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


//! \}
