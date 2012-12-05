// $Id$
/*
 * Event Handler for MIDIbox NG
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
#include <string.h>

#include "app.h"
#include "mbng_event.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_matrix.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef struct { // should be dividable by u16
  u16 id;
  u16 enabled_ports;
  u16 flags; // (mbng_event_flags_t)
  u16 min;
  u16 max;
  u8 len; // for the whole item. positioned here, so that u16 entries are halfword aligned
  u8 len_stream;
  u8 len_label;
  u8 lcd_num;
  u8 lcd_pos;
  u8 data_begin; // data section for streams and label start here, it can have multiple bytes
} mbng_event_pool_item_t;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define MBNG_EVENT_POOL_MAX_SIZE 8192
static u8 event_pool[MBNG_EVENT_POOL_MAX_SIZE];
static u16 event_pool_size;
static u16 event_pool_num_items;


/////////////////////////////////////////////////////////////////////////////
// This function initializes the event pool structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  MBNG_EVENT_PoolClear();

  mbng_event_item_t item;
  MBNG_EVENT_ItemInit(&item);

  // Default Items:
  int i;

  // Buttons
  for(i=0; i<64; ++i) {
    char str[21];
    u8 stream[20];

    item.id = (u16)MBNG_EVENT_CONTROLLER_BUTTON + i;

    item.flags.general.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i;
    item.stream = stream;
    item.stream_size = 2;

    sprintf(str, "Button #%d", i+1);
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // LEDs
  for(i=0; i<64; ++i) {
    char str[21];
    u8 stream[20];

    item.id = (u16)MBNG_EVENT_CONTROLLER_LED + i;

    item.flags.general.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i;
    item.stream = stream;
    item.stream_size = 2;

    sprintf(str, "LED #%d", i+1);
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // Encoders
  for(i=0; i<64; ++i) {
    char str[21];
    u8 stream[20];

    item.id = (u16)MBNG_EVENT_CONTROLLER_ENC + i;

    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb0;
    stream[1] = 0x10 + i;
    item.stream = stream;
    item.stream_size = 2;

    sprintf(str, "ENC #%d", i+1);
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINSER
  for(i=0; i<64; ++i) {
    char str[21];
    u8 stream[20];

    item.id = (u16)MBNG_EVENT_CONTROLLER_AINSER + i;
    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb1;
    stream[1] = 0x10 + i;
    item.stream = stream;
    item.stream_size = 2;

    sprintf(str, "AINSER #%d", i+1);
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINs
  for(i=0; i<6; ++i) {
    char str[21];
    u8 stream[20];

    item.id = (u16)MBNG_EVENT_CONTROLLER_AIN + i;

    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb2;
    stream[1] = 0x10 + i;
    item.stream = stream;
    item.stream_size = 2;

    sprintf(str, "AIN #%d", i+1);
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Clear the event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolClear(void)
{
  event_pool_size = 0;
  event_pool_num_items = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sends the event pool to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolPrint(void)
{
  return MIOS32_MIDI_SendDebugHexDump(event_pool, event_pool_size);
}

/////////////////////////////////////////////////////////////////////////////
// returns the current pool size
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolNumItemsGet(void)
{
  return event_pool_num_items;
}

/////////////////////////////////////////////////////////////////////////////
// returns the current pool size
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolSizeGet(void)
{
  return event_pool_size;
}

/////////////////////////////////////////////////////////////////////////////
// returns the maximum pool size
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolMaxSizeGet(void)
{
  return MBNG_EVENT_POOL_MAX_SIZE;
}


/////////////////////////////////////////////////////////////////////////////
// Initializes an item with default settings
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item)
{
  item->id = (u16)MBNG_EVENT_CONTROLLER_DISABLED;
  item->flags.ALL = 0;
  item->enabled_ports = 0x1011; // OSC1, UART1 and USB1
  item->min = 0x0000;
  item->max = 0x007f;
  item->stream_size = 0;
  item->lcd_num = 0;
  item->lcd_pos = 0x00;
  item->stream = NULL;
  item->label = NULL;
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Local function to copy a pool item into a "user" item
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2User(mbng_event_pool_item_t* pool_item, mbng_event_item_t *item)
{
  item->id = pool_item->id;
  item->flags.ALL = pool_item->flags;
  item->enabled_ports = pool_item->enabled_ports;
  item->min = pool_item->min;
  item->max = pool_item->max;
  item->lcd_num = pool_item->lcd_num;
  item->lcd_pos = pool_item->lcd_pos;
  item->stream_size = pool_item->len_stream;
  item->stream = pool_item->len_stream ? (u8 *)&pool_item->data_begin : NULL;
  item->label = pool_item->len_label ? ((char *)&pool_item->data_begin + pool_item->len_stream) : NULL;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Local function to copy a "user" item into a "pool" item
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2Pool(mbng_event_item_t *item, mbng_event_pool_item_t* pool_item)
{
  u32 label_len = item->label ? (strlen(item->label)+1) : 0;
  u32 pool_item_len = sizeof(mbng_event_pool_item_t) - 1 + item->stream_size + label_len;

  pool_item->id = item->id;
  pool_item->flags = item->flags.ALL;
  pool_item->enabled_ports = item->enabled_ports;
  pool_item->min = item->min;
  pool_item->max = item->max;
  pool_item->lcd_num = item->lcd_num;
  pool_item->lcd_pos = item->lcd_pos;
  pool_item->len = pool_item_len;
  pool_item->len_stream = item->stream ? item->stream_size : 0;
  pool_item->len_label = label_len;

  if( pool_item->len_stream )
    memcpy((u8 *)&pool_item->data_begin, item->stream, pool_item->len_stream);

  if( pool_item->len_label )
    memcpy((u8 *)&pool_item->data_begin + pool_item->len_stream, item->label, pool_item->len_label);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Adds an item to event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemAdd(mbng_event_item_t *item)
{
  u32 label_len = item->label ? (strlen(item->label)+1) : 0;
  u32 pool_item_len = sizeof(mbng_event_pool_item_t) - 1 + item->stream_size + label_len;

  if( pool_item_len > 255 )
    return -1; // too much data

  if( (event_pool_size+pool_item_len) > MBNG_EVENT_POOL_MAX_SIZE )
    return -1; // out of storage 

  mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)&event_pool[event_pool_size];
  MBNG_EVENT_ItemCopy2Pool(item, pool_item);
  event_pool_size += pool_item->len;
  ++event_pool_num_items;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Search an item in event pool based on ID
// returns 0 and copies item into *item if found
// returns -1 if item not found
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSearchById(mbng_event_item_id_t id, mbng_event_item_t *item)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->id == id ) {
      MBNG_EVENT_ItemCopy2User(pool_item, item);
      return 0; // item found
    }
    pool_ptr += pool_item->len;
  }

  return -1; // not found
}

/////////////////////////////////////////////////////////////////////////////
// Sends the an item description to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemPrint(mbng_event_item_t *item)
{
  return MIOS32_MIDI_SendDebugMessage("[EVENT:%04x] %s %s ports:%04x min:%d max:%d label:%s\n",
				      item->id,
				      MBNG_EVENT_ItemControllerStrGet(item),
				      MBNG_EVENT_ItemTypeStrGet(item),
				      item->enabled_ports,
				      item->min,
				      item->max,
				      item->label ? item->label : "");
}

/////////////////////////////////////////////////////////////////////////////
// returns name of controller
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_t *item)
{
  switch( item->id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_DISABLED:      return "DISABLED";
  case MBNG_EVENT_CONTROLLER_BUTTON:        return "BUTTON";
  case MBNG_EVENT_CONTROLLER_LED:           return "LED";
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: return "BUTTON_MATRIX";
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    return "LED_MATRIX";
  case MBNG_EVENT_CONTROLLER_ENC:           return "ENC";
  case MBNG_EVENT_CONTROLLER_AIN:           return "AIN";
  case MBNG_EVENT_CONTROLLER_AINSER:        return "AINSER";
  case MBNG_EVENT_CONTROLLER_MF:            return "MF";
  case MBNG_EVENT_CONTROLLER_CV:            return "CV";
  }
  return "RESERVED";
}

/////////////////////////////////////////////////////////////////////////////
// Sends the an item description to debug terminal
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item)
{
  switch( item->flags.general.type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       return "NOTE_OFF";
  case MBNG_EVENT_TYPE_NOTE_ON:        return "NOTE_ON";
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  return "POLY_PRESSURE";
  case MBNG_EVENT_TYPE_CC:             return "CC";
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: return "PROGRAM_CHANGE";
  case MBNG_EVENT_TYPE_AFTERTOUCH:     return "AFTERTOUCH";
  case MBNG_EVENT_TYPE_PITCHBEND:      return "PITCHBEND";
  case MBNG_EVENT_TYPE_SYSEX:          return "SYSEX";
  case MBNG_EVENT_TYPE_RPN:            return "RPN";
  case MBNG_EVENT_TYPE_NRPN:           return "NRPN";
  case MBNG_EVENT_TYPE_META:           return "META";
  }
  return "RESERVED";
}


/////////////////////////////////////////////////////////////////////////////
// Sends the item via MIDI
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item, u16 value)
{
  if( !item->stream_size )
    return 0; // nothing to send

  mbng_event_type_t event_type = item->flags.general.type;
  if( event_type <= MBNG_EVENT_TYPE_PITCHBEND ) {
    u8 event = (item->stream[0] >> 4) | 8;

    // create MIDI package
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = event;
    p.event = event;

    if( mbng_patch_cfg.global_chn )
      p.chn = mbng_patch_cfg.global_chn - 1;
    else
      p.chn = item->stream[0] & 0xf;

    switch( event_type ) {
    case MBNG_EVENT_TYPE_PROGRAM_CHANGE:
    case MBNG_EVENT_TYPE_AFTERTOUCH:
      p.evnt1 = value & 0x7f;
      p.evnt2 = 0;
      break;

    case MBNG_EVENT_TYPE_PITCHBEND:
      if( (item->max-item->min) < 128 ) { // 7bit range
	value &= 0x7f; // just to ensure
	p.evnt1 = (value == 0x40) ? 0x00 : value;
	p.evnt2 = value;
      } else {
	p.evnt1 = (value & 0x7f);
	p.evnt2 = (value >> 7) & 0x7f;
      }
      break;

    default:
      p.evnt1 = item->stream[1] & 0x7f;
      p.evnt2 = value & 0x7f;
    }

    // send MIDI package over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	MIOS32_MIDI_SendPackage(port, p);
      }
    }
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_RPN ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_META ) {
    // TODO
  } else {
    return -1; // unsupported
  }

  return -1; // unsupported item type
}


/////////////////////////////////////////////////////////////////////////////
// Called when an item should be notified
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value)
{
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_DEBUG ) {
    MBNG_EVENT_ItemPrint(item);
  }

  switch( item->id & 0xf000 ) {
  //case MBNG_EVENT_CONTROLLER_DISABLED:
  case MBNG_EVENT_CONTROLLER_BUTTON:        return MBNG_DIN_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_LED:           return MBNG_DOUT_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: return MBNG_MATRIX_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    return -1; // TODO
  case MBNG_EVENT_CONTROLLER_ENC:           return MBNG_ENC_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_AIN:           return MBNG_AIN_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_AINSER:        return MBNG_AIN_NotifyReceivedValue_SER64(item, value);
  case MBNG_EVENT_CONTROLLER_MF:            return -1; // TODO
  case MBNG_EVENT_CONTROLLER_CV:            return -1; // TODO
  }

  return -1; // unsupported controller type
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_MIDI_NotifyPackage whenver a new
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // check for "all notes off" command
  if( midi_package.event == CC &&
      mbng_patch_cfg.all_notes_off_chn &&
      (midi_package.chn == (mbng_patch_cfg.all_notes_off_chn - 1)) &&
      midi_package.cc_number == 123 ) {
    MBNG_DOUT_Init(0);
  }

  // search in pool for matching events
  u8 evnt0 = midi_package.evnt0;
  u8 evnt1 = midi_package.evnt1;
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->data_begin == evnt0 && pool_item->len_stream ) { // timing critical
      // first byte is matching - now we've a bit more time for checking
      
      mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).general.type;
      if( event_type <= MBNG_EVENT_TYPE_CC ) {
	u8 *stream = &pool_item->data_begin;
	if( evnt1 == stream[1] ) {
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  MBNG_EVENT_ItemReceive(&item, midi_package.value);
	}
      } else if( event_type <= MBNG_EVENT_TYPE_AFTERTOUCH ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1);
      } else if( event_type == MBNG_EVENT_TYPE_PITCHBEND ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1 | ((u16)midi_package.value << 7));
      } else {
	// TODO
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}
