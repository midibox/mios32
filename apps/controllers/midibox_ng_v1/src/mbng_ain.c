// $Id$
/*
 * AIN access functions for MIDIbox NG
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
#include "mbng_ain.h"
#include "mbng_lcd.h"
#include "mbng_patch.h"
#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static u8 ain_group;
static u8 ainser_group;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  ain_group = 0;
  ainser_group = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_AIN_NotifyChange when an input
// has changed its value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange(u32 pin, u32 pin_value)
{
  if( pin >= MBNG_PATCH_NUM_AIN )
    return -1; // invalid pin

  // tmp. as long as no AIN HW enable provided
  return 0;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyChange(%d, %d)\n", pin, pin_value);
  }

  // search for AIN
  int ain_ix = ain_group * mbng_patch_cfg.ain_group_size + pin;
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(MBNG_EVENT_CONTROLLER_AIN + ain_ix + 1, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AIN_IX %d\n", ain_ix);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = item.min + (((256*pin_value)/4096) * (item.max-item.min+1) / 256);

  // TODO: handle AIN modes

  // print label
  MBNG_LCD_PrintItemLabel(&item, value);

  // send MIDI event
  return MBNG_EVENT_ItemSend(&item, value);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an AINSER pot has been moved
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyChange_SER64(u32 module, u32 pin, u32 pin_value)
{
  int mbng_pin = module*64 + pin;
  if( mbng_pin >= MBNG_PATCH_NUM_AINSER )
    return -1; // invalid pin

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyChange_SER64(%d, %d, %d)\n", module, pin, pin_value);
  }

  // search for AIN
  int ain_ix = ain_group * mbng_patch_cfg.ain_group_size + pin;
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(MBNG_EVENT_CONTROLLER_AINSER + ain_ix + 1, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AINSER_IX %d\n", ain_ix);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = item.min + (((256*pin_value)/4096) * (item.max-item.min+1) / 256);

  // TODO: handle AIN modes

  // print label
  MBNG_LCD_PrintItemLabel(&item, value);

  // send MIDI event
  return MBNG_EVENT_ItemSend(&item, value);
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyReceivedValue(mbng_event_item_t *item, u16 value)
{
  int ain_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyReceivedValue(%d, %d)\n", ain_ix, value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyReceivedValue_SER64(mbng_event_item_t *item, u16 value)
{
  int ain_ix = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AINSER64_NotifyReceivedValue(%d, %d)\n", ain_ix, value);
  }

  return 0; // no error
}
