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

static u16 ain_value[MBNG_PATCH_NUM_AIN];
static u16 ainser_value[MBNG_PATCH_NUM_AINSER];

/////////////////////////////////////////////////////////////////////////////
// This function initializes the AIN handler
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  int i;
  for(i=0; i<MBNG_PATCH_NUM_AIN; ++i)
    ain_value[i] = 0;

  for(i=0; i<MBNG_PATCH_NUM_AINSER; ++i)
    ainser_value[i] = 0;

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

  // get ID
  mbng_event_item_id_t ain_id = MBNG_EVENT_CONTROLLER_AIN + pin + 1;
  MBNG_PATCH_BankCtrlIdGet(pin, &ain_id); // modifies id depending on bank selection
  mbng_event_item_t item;
  if( MBNG_EVENT_ItemSearchById(ain_id, &item) < 0 ) {
    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("No event assigned to AIN id=%d\n", ain_id & 0xfff);
    }
    return -2; // no event assigned
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    MBNG_EVENT_ItemPrint(&item);
  }

  // scale 12bit value between min/max with fixed point artithmetic
  int value = item.min + (((256*pin_value)/4096) * (item.max-item.min+1) / 256);

  int ain_ix = (ain_id & 0xfff) - 1;
  if( ain_ix >= 0 || ain_ix < MBNG_PATCH_NUM_AIN )
    ain_value[ain_ix] = value;
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

  // get ID
  mbng_event_item_id_t ainser_id = MBNG_EVENT_CONTROLLER_AINSER + pin + 1;
  MBNG_PATCH_BankCtrlIdGet(pin, &ainser_id); // modifies id depending on bank selection
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
  int value = item.min + (((256*pin_value)/4096) * (item.max-item.min+1) / 256);

  int ainser_ix = (ainser_id & 0xfff) - 1;
  if( ainser_ix >= 0 || ainser_ix < MBNG_PATCH_NUM_AIN )
    ainser_value[ainser_ix] = value;
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
  int ain_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyReceivedValue(%d, %d)\n", ain_subid, value);
  }

  // store new value
  if( ain_subid && ain_subid <= MBNG_PATCH_NUM_AIN )
    ain_value[ain_subid-1] = value;

  // forward
  if( item->fwd_id && (!MBNG_PATCH_BankCtrlInBank(item) || MBNG_PATCH_BankCtrlIsActive(item)) )
    MBNG_EVENT_ItemForward(item, value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_Refresh() to refresh the controller
// (mainly to trigger the forward item)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyRefresh(mbng_event_item_t *item)
{
  int ain_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyRefresh(%d)\n", ain_subid);
  }

  if( ain_subid && ain_subid <= MBNG_PATCH_NUM_AIN ) {
    u16 value = ain_value[ain_subid-1];
    MBNG_AIN_NotifyReceivedValue(item, value);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_ItemReceive when a matching value
// has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyReceivedValue_SER64(mbng_event_item_t *item, u16 value)
{
  int ainser_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyReceivedValue_SER64(%d, %d)\n", ainser_subid, value);
  }

  // store new value
  if( ainser_subid && ainser_subid <= MBNG_PATCH_NUM_AINSER )
    ainser_value[ainser_subid-1] = value;

  // forward
  if( item->fwd_id && (!MBNG_PATCH_BankCtrlInBank(item) || MBNG_PATCH_BankCtrlIsActive(item)) )
    MBNG_EVENT_ItemForward(item, value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MBNG_EVENT_Refresh() to refresh the controller
// (mainly to trigger the forward item)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_AIN_NotifyRefresh_SER64(mbng_event_item_t *item)
{
  int ainser_subid = item->id & 0xfff;

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
    DEBUG_MSG("MBNG_AIN_NotifyRefresh_SER64(%d)\n", ainser_subid);
  }

  if( ainser_subid && ainser_subid <= MBNG_PATCH_NUM_AINSER ) {
    u16 value = ainser_value[ainser_subid-1];
    MBNG_AIN_NotifyReceivedValue_SER64(item, value);
  }

  return 0; // no error
}
