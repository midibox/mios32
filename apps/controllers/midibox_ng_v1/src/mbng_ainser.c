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
      if( ainser->flags.enabled && ainser->flags.cs == module )
	break;
    }

    if( i >= MBNG_PATCH_NUM_AINSER_MODULES )
      return -2; // module not mapped

    mapped_module = i;
  }

  int mbng_pin = mapped_module*64 + pin;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyChange(%d, %d, %d)\n", mapped_module, mbng_pin, pin_value);
  }

  // get ID
  mbng_event_item_id_t ainser_id = MBNG_EVENT_CONTROLLER_AINSER + mbng_pin + 1;
  MBNG_PATCH_BankCtrlIdGet(mbng_pin, &ainser_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(ainser_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AINSER id=%d\n", ainser_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = pin_value;
  if( item.min <= item.max ) {
    value = item.min + (((256*value)/4096) * (item.max-item.min+1) / 256);
  } else {
    value = item.min - (((256*value)/4096) * (item.min-item.max+1) / 256);
  }

  int ainser_ix = (ainser_id & 0xfff) - 1;
  if( ainser_ix >= 0 || ainser_ix < MBNG_PATCH_NUM_AIN ) {
    int prev_value = AINSER_PreviousPinValueGet();
    if( item.min <= item.max ) {
      prev_value = item.min + (((256*prev_value)/4096) * (item.max-item.min+1) / 256);
    } else {
      prev_value = item.min - (((256*prev_value)/4096) * (item.min-item.max+1) / 256);
    }

    if( MBNG_AIN_HandleAinMode(&item, value, prev_value) < 0 )
      return 0; // don't send
  }

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
  int ain_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER_NotifyReceivedValue(%d, %d)\n", ain_subid, item->value);
  }

  // nothing else to do...

  return 0; // no error
}
