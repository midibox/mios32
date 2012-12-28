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

#include <scs.h>
#include <ainser.h>

#include "app.h"
#include "tasks.h"
#include "mbng_event.h"
#include "mbng_lcd.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_matrix.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
#include "mbng_ainser.h"
#include "mbng_mf.h"
#include "mbng_cv.h"
#include "mbng_patch.h"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;

  struct {
    u8 sysex_dump;
  };
} runtime_flags_t;

typedef struct { // should be dividable by u16
  u16 id;
  u16 enabled_ports;
  mbng_event_flags_t flags;
  u16 fwd_id;
  u16 value;
  s16 min;
  s16 max;
  s16 offset;
  mbng_event_syxdump_pos_t syxdump_pos;
  u16 tmp_sysex_match_ctr; // temporary variable which counts matching bytes in SysEx stream
  u16 tmp_sysex_value;     // temporary variable which stores current SysEx value
  runtime_flags_t tmp_runtime_flags;    // several runtime flags
  u8 len; // for the whole item. positioned here, so that u16 entries are halfword aligned
  u8 len_stream;
  u8 len_label;
  u8 map;
  u8 secondary_value;
  u8 lcd;
  u8 lcd_pos;
  u8 data_begin; // data section for streams and label starts here, it can have multiple bytes
} mbng_event_pool_item_t;

typedef struct {
  u8 len;
  u8 num;
  u8 data_begin; // data section for value map starts here, it can have multiple bytes
} mbng_event_pool_map_t;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


#define MBNG_EVENT_POOL_MAX_SIZE (24*1024)

#ifndef AHB_SECTION
#define AHB_SECTION
#endif

static u8 AHB_SECTION event_pool[MBNG_EVENT_POOL_MAX_SIZE];
static u16 event_pool_size;
static u16 event_pool_maps_begin;
static u16 event_pool_num_items;
static u16 event_pool_num_maps;

// listen to NRPN for up to 8 ports at up to 16 channels
// in order to save RAM, we only listen to USB and UART based ports! (this already costs us 512 byte!)
#define MBNG_EVENT_NRPN_RECEIVE_PORTS_MASK    0x00ff
#define MBNG_EVENT_NRPN_RECEIVE_PORTS_OFFSET  0
#define MBNG_EVENT_NRPN_RECEIVE_PORTS         8
#define MBNG_EVENT_NRPN_RECEIVE_CHANNELS     16
static u16 nrpn_received_address[MBNG_EVENT_NRPN_RECEIVE_PORTS][MBNG_EVENT_NRPN_RECEIVE_CHANNELS];
static u16 nrpn_received_value[MBNG_EVENT_NRPN_RECEIVE_PORTS][MBNG_EVENT_NRPN_RECEIVE_CHANNELS];

// for UART based transfers we also optimize the output
#define MBNG_EVENT_NRPN_SEND_PORTS_MASK    0x00f0
#define MBNG_EVENT_NRPN_SEND_PORTS_OFFSET  4
#define MBNG_EVENT_NRPN_SEND_PORTS         4
#define MBNG_EVENT_NRPN_SEND_CHANNELS     16
static u16 nrpn_sent_address[MBNG_EVENT_NRPN_SEND_PORTS][MBNG_EVENT_NRPN_SEND_CHANNELS];
static u16 nrpn_sent_value[MBNG_EVENT_NRPN_SEND_PORTS][MBNG_EVENT_NRPN_SEND_CHANNELS];


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2User(mbng_event_pool_item_t* pool_item, mbng_event_item_t *item);
static s32 MBNG_EVENT_ItemCopy2Pool(mbng_event_item_t *item, mbng_event_pool_item_t* pool_item);


/////////////////////////////////////////////////////////////////////////////
// This function initializes the event pool structure
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  MBNG_EVENT_PoolClear();

  {
    int i, j;

    u16 *nrpn_received_address_ptr = (u16 *)&nrpn_received_address[0][0];
    u16 *nrpn_received_value_ptr = (u16 *)&nrpn_received_value[0][0];
    for(i=0; i<MBNG_EVENT_NRPN_RECEIVE_PORTS; ++i) {
      for(j=0; j<MBNG_EVENT_NRPN_RECEIVE_CHANNELS; ++j) {
	*nrpn_received_address_ptr = 0;
	*nrpn_received_value_ptr = 0;
      }
    }

    u16 *nrpn_sent_address_ptr = (u16 *)&nrpn_sent_address[0][0];
    u16 *nrpn_sent_value_ptr = (u16 *)&nrpn_sent_value[0][0];
    for(i=0; i<MBNG_EVENT_NRPN_SEND_PORTS; ++i) {
      for(j=0; j<MBNG_EVENT_NRPN_SEND_CHANNELS; ++j) {
	*nrpn_sent_address_ptr = 0xffff; // invalidate
	*nrpn_sent_value_ptr = 0xffff; // invalidate
      }
    }
  }

  mbng_event_item_t item;

  // Default Items:
  int i;

  // Buttons
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_BUTTON + i);
    item.fwd_id = MBNG_EVENT_CONTROLLER_LED | i;
    item.flags.general.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_btn"); // Button #%3i %3d%b
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // Encoders
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_ENC + i);
    item.fwd_id = MBNG_EVENT_CONTROLLER_LED_MATRIX | i;
    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb0;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_enc"); // ENC #%3i    %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINSER
  AINSER_EnabledSet(0, 1);
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_AINSER + i);
    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb1;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_aser"); // AINSER #%3i %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  // AINs
  for(i=1; i<=6; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_AIN + i);
    item.flags.general.type = MBNG_EVENT_TYPE_CC;
    stream[0] = 0xb2;
    stream[1] = 0x10 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_ain"); // AIN #%3i    %3d%B
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called each mS (with low priority)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Tick(void)
{
  static u16 ms_counter = 0;

  // each second:
  if( ++ms_counter < 1000 )
    return 0; // no error
  ms_counter = 0;

  // invalidate the NRPN optimizer
  portENTER_CRITICAL(); // should be atomic!
  {
    int i, j;

    u16 *nrpn_sent_address_ptr = (u16 *)&nrpn_sent_address[0][0];
    u16 *nrpn_sent_value_ptr = (u16 *)&nrpn_sent_value[0][0];
    for(i=0; i<MBNG_EVENT_NRPN_SEND_PORTS; ++i) {
      for(j=0; j<MBNG_EVENT_NRPN_SEND_CHANNELS; ++j) {
	*nrpn_sent_address_ptr = 0xffff; // invalidate
	*nrpn_sent_value_ptr = 0xffff; // invalidate
      }
    }
  }
  portEXIT_CRITICAL();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear the event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolClear(void)
{
  event_pool_size = 0;
  event_pool_maps_begin = 0;
  event_pool_num_items = 0;
  event_pool_num_maps = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Called after a new file has been loaded (post-processing step)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolUpdate(void)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    int map_len;
    u8 *map_values;
    if( pool_item->map && (map_len=MBNG_EVENT_MapGet(pool_item->map, &map_values)) > 0 ) {
      // determine min/max value of map
      int pos;
      u8 min = 255;
      u8 max = 0;
      u8 *map_values_ptr = map_values;
      for(pos=0; pos<map_len; ++pos, ++map_values_ptr) {
	if( *map_values_ptr >= max )
	  max = *map_values_ptr;
	if( *map_values_ptr < min )
	  min = *map_values_ptr;
      }

      // transfer min/max to pool item
      pool_item->min = min;
      pool_item->max = max;
    }

    pool_ptr += pool_item->len;
  }

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
// Sends short item informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolItemsPrint(void)
{
  if( !event_pool_num_items ) {
    DEBUG_MSG("No Events in pool\n");
  } else {
    DEBUG_MSG("%d Events in pool:\n", event_pool_num_items);
    u8 *pool_ptr = (u8 *)&event_pool[0];
    u32 i;
    for(i=0; i<event_pool_num_items; ++i) {
      mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemPrint(&item);

      pool_ptr += pool_item->len;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends short map informations to debug terminal
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolMapsPrint(void)
{
  if( !event_pool_num_maps ) {
    DEBUG_MSG("No Maps in pool\n");
  } else {
    DEBUG_MSG("%d Maps in pool:\n", event_pool_num_maps);
    u8 *pool_ptr = (u8 *)&event_pool[event_pool_maps_begin];
    u32 i;
    for(i=0; i<event_pool_num_maps; ++i) {
      mbng_event_pool_map_t *pool_map = (mbng_event_pool_map_t *)pool_ptr;

      char value_str[128];
      sprintf(value_str, "MAP%d", pool_map->num);

      u8 *map_values = (u8 *)&pool_map->data_begin;
      int j;
      for(j=2; j<pool_map->len && j < 16; ++j ) {
	sprintf(value_str, "%s %d", value_str, *map_values++);
      }

      if( j == 16 ) {
	sprintf(value_str, "%s ...", value_str);
      }
      DEBUG_MSG(value_str);

      pool_ptr += pool_map->len;
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// returns the number of EVENT items
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolNumItemsGet(void)
{
  return event_pool_num_items;
}

/////////////////////////////////////////////////////////////////////////////
// returns the number of MAPs
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_PoolNumMapsGet(void)
{
  return event_pool_num_maps;
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
// Adds a map to event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapAdd(u8 map, u8 *map_values, u8 len)
{
  if( (event_pool_size+len+2) > MBNG_EVENT_POOL_MAX_SIZE )
    return -2; // out of storage 

  u32 event_pool_map_start = event_pool_size;

  ++event_pool_num_maps;
  u8 *pool_ptr = (u8 *)&event_pool[event_pool_map_start];
  *pool_ptr++ = len+2;
  *pool_ptr++ = map;
  memcpy(pool_ptr, (u8 *)map_values, len);

  event_pool_size += len + 2;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns a map from the event pool
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapGet(u8 map, u8 **map_values)
{
  u8 *pool_ptr = (u8 *)&event_pool[event_pool_maps_begin];
  u32 i;
  for(i=0; i<event_pool_num_maps; ++i) {
      mbng_event_pool_map_t *pool_map = (mbng_event_pool_map_t *)pool_ptr;

      if( pool_map->num == map ) {
	*map_values = (u8 *)&pool_map->data_begin;
	return pool_map->len - 2;
      }

      pool_ptr += pool_map->len;
  }

  return -1; // map not available
}

/////////////////////////////////////////////////////////////////////////////
// Returns the index of a given value
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MapIxGet(u8 *map_values, u8 map_len, u8 value)
{
  // first search for exact match
  {
    int i;
    u8 *map_values_ptr = map_values;
    for(i=0; i<map_len; ++i)
      if( *map_values_ptr++ == value )
	return i;
  }

  // otherwise search for match which is close to the given value
  {
    int i;
    u8 *map_values_ptr = map_values;
    for(i=0; i<map_len; ++i)
      if( *map_values_ptr++ > value ) {
	return (i == 0) ? 0 : (i-1);
      }
  }

  return map_len-1; // no match -> take last index
}

/////////////////////////////////////////////////////////////////////////////
// Initializes an item with default settings
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item, mbng_event_item_id_t id)
{
  item->id = id;
  item->pool_address = 0xffff; // invalid address
  item->flags.ALL = 0;
  item->enabled_ports = 0x1011; // OSC1, UART1 and USB1
  item->fwd_id = 0;
  item->value  = 0;
  item->min    = 0;
  item->max    = 127;
  item->offset = 0;
  item->syxdump_pos.ALL = 0;
  item->stream_size = 0;
  item->map = 0;
  item->secondary_value = 0;
  item->lcd = 0;
  item->lcd_pos = 0x00;
  item->stream = NULL;
  item->label = NULL;

  // differ between type
  switch( id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_SENDER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_BUTTON: {
    item->flags.DIN.button_mode = MBNG_EVENT_BUTTON_MODE_ON_OFF;
  }; break;

  case MBNG_EVENT_CONTROLLER_LED: {
  }; break;

  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
  }; break;

  case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
    item->flags.LED_MATRIX.led_matrix_pattern = MBNG_EVENT_LED_MATRIX_PATTERN_1;
  }; break;

  case MBNG_EVENT_CONTROLLER_ENC: {
    item->flags.ENC.led_matrix_pattern = MBNG_EVENT_LED_MATRIX_PATTERN_1;
    item->flags.ENC.enc_mode = MBNG_EVENT_ENC_MODE_ABSOLUTE;
    item->flags.ENC.enc_speed_mode = MBNG_EVENT_ENC_SPEED_MODE_AUTO;
  }; break;

  case MBNG_EVENT_CONTROLLER_AIN: {
  }; break;

  case MBNG_EVENT_CONTROLLER_AINSER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_MF: {
  }; break;

  case MBNG_EVENT_CONTROLLER_CV: {
  }; break;
  }
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Local function to copy a pool item into a "user" item
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_ItemCopy2User(mbng_event_pool_item_t* pool_item, mbng_event_item_t *item)
{
  // note: sooner or later I will clean-up this copy routine!
  // It copies many parts of the mbng_event_pool_item_t structure into the mbng_event_item_t
  // with accesses to all structure members. 
  // Each copy operation consists of two instructions (see project_build/project.lss)
  // regardless if 32, 16 or 8 bit variables are copied.
  // Optimisation could be done by arranging struct members in a way which allows direct 32bit
  // copy operations.
  // Maintaining two structs this way is "dirty", but really helps to improve the performance (a bit)
  // on this timing critical part.
  // This should be done once the struct definitions are settled - currently it isn't urgent
  // since I haven't noticed significant performnance issues yet (on a LPC17 core...)
  item->id = pool_item->id;
  u32 pool_address = (u32)pool_item - (u32)event_pool;
  item->pool_address = (pool_address < MBNG_EVENT_POOL_MAX_SIZE) ? pool_address : 0xffff;
  item->flags.ALL = pool_item->flags.ALL;
  item->enabled_ports = pool_item->enabled_ports;
  item->fwd_id = pool_item->fwd_id;
  item->value = pool_item->value;
  item->min = pool_item->min;
  item->max = pool_item->max;
  item->offset = pool_item->offset;
  item->matrix_pin = 0; // has to be set after creation by the MATRIX handler
  item->syxdump_pos.ALL = pool_item->syxdump_pos.ALL;
  item->map = pool_item->map;
  item->secondary_value = pool_item->secondary_value;
  item->lcd = pool_item->lcd;
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
  pool_item->flags.ALL = item->flags.ALL;
  pool_item->enabled_ports = item->enabled_ports;
  pool_item->fwd_id = item->fwd_id;
  pool_item->value = item->value;
  pool_item->min = item->min;
  pool_item->max = item->max;
  pool_item->offset = item->offset;
  pool_item->syxdump_pos.ALL = item->syxdump_pos.ALL;
  pool_item->map = item->map;
  pool_item->secondary_value = item->secondary_value;
  pool_item->lcd = item->lcd;
  pool_item->lcd_pos = item->lcd_pos;
  pool_item->len = pool_item_len;
  pool_item->len_stream = item->stream ? item->stream_size : 0;
  pool_item->len_label = label_len;

  if( pool_item->len_stream )
    memcpy((u8 *)&pool_item->data_begin, item->stream, pool_item->len_stream);

  if( pool_item->len_label )
    memcpy((u8 *)&pool_item->data_begin + pool_item->len_stream, item->label, pool_item->len_label);

  // temporary variables:
  pool_item->tmp_sysex_match_ctr = 0;
  pool_item->tmp_sysex_value = 0;
  pool_item->tmp_runtime_flags.ALL = 0;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Returns an item of the event pool with the given index number
// (only used for dumping out all items in mbng_file_p.c)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemGet(u32 item_ix, mbng_event_item_t *item)
{
  if( item_ix >= event_pool_num_items )
    return -1; // not found

  u8 *pool_ptr = (u8 *)&event_pool[0];
  mbng_event_pool_item_t *pool_item;

  u32 i;
  for(i=0; i<=item_ix; ++i) {
    pool_item = (mbng_event_pool_item_t *)pool_ptr;
    pool_ptr += pool_item->len;
  }

  MBNG_EVENT_ItemCopy2User(pool_item, item);
  return 0; // item found
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
    return -2; // out of storage 

  // shift map items
  if( event_pool_maps_begin < event_pool_size ) {
    u8 *first_map = (u8 *)&event_pool[event_pool_maps_begin];
    u8 *new_map_begin = (u8 *)&event_pool[event_pool_maps_begin + pool_item_len];
    u32 map_items_size = event_pool_size - event_pool_maps_begin;
    memmove(new_map_begin, first_map, map_items_size);
  }

  mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)&event_pool[event_pool_maps_begin];
  MBNG_EVENT_ItemCopy2Pool(item, pool_item);
  event_pool_size += pool_item->len;
  ++event_pool_num_items;
  event_pool_maps_begin += pool_item_len;

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
#if 0
  MIOS32_MIDI_SendDebugMessage("[EVENT:%04x] %s %s stream:",
			       item->id,
			       MBNG_EVENT_ItemControllerStrGet(item->id),
			       MBNG_EVENT_ItemTypeStrGet(item));
  if( item->stream_size ) {
    MIOS32_MIDI_SendDebugHexDump(item->stream, item->stream_size);
  }
  return 0;
#else
  return MIOS32_MIDI_SendDebugMessage("[EVENT:%04x] %s %s ports:%04x fwd_id:0x%04x min:%d max:%d label:%s\n",
				      item->id,
				      MBNG_EVENT_ItemControllerStrGet(item->id),
				      MBNG_EVENT_ItemTypeStrGet(item),
				      item->enabled_ports,
				      item->fwd_id,
				      item->min,
				      item->max,
				      item->label ? item->label : "");
#endif
}

/////////////////////////////////////////////////////////////////////////////
// returns name of controller
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_id_t id)
{
  switch( id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_DISABLED:      return "DISABLED";
  case MBNG_EVENT_CONTROLLER_SENDER:        return "SENDER";
  case MBNG_EVENT_CONTROLLER_RECEIVER:      return "RECEIVER";
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
  return "DISABLED";
}

/////////////////////////////////////////////////////////////////////////////
// returns ID of controller given as string
/////////////////////////////////////////////////////////////////////////////
mbng_event_item_id_t MBNG_EVENT_ItemIdFromControllerStrGet(char *event)
{
  if( strcasecmp(event, "SENDER") == 0 )        return MBNG_EVENT_CONTROLLER_SENDER;
  if( strcasecmp(event, "RECEIVER") == 0 )      return MBNG_EVENT_CONTROLLER_RECEIVER;
  if( strcasecmp(event, "BUTTON") == 0 )        return MBNG_EVENT_CONTROLLER_BUTTON;
  if( strcasecmp(event, "LED") == 0 )           return MBNG_EVENT_CONTROLLER_LED;
  if( strcasecmp(event, "BUTTON_MATRIX") == 0 ) return MBNG_EVENT_CONTROLLER_BUTTON_MATRIX;
  if( strcasecmp(event, "LED_MATRIX") == 0 )    return MBNG_EVENT_CONTROLLER_LED_MATRIX;
  if( strcasecmp(event, "ENC") == 0 )           return MBNG_EVENT_CONTROLLER_ENC;
  if( strcasecmp(event, "AIN") == 0 )           return MBNG_EVENT_CONTROLLER_AIN;
  if( strcasecmp(event, "AINSER") == 0 )        return MBNG_EVENT_CONTROLLER_AINSER;
  if( strcasecmp(event, "MF") == 0 )            return MBNG_EVENT_CONTROLLER_MF;
  if( strcasecmp(event, "CV") == 0 )            return MBNG_EVENT_CONTROLLER_CV;

  return MBNG_EVENT_CONTROLLER_DISABLED;
}

/////////////////////////////////////////////////////////////////////////////
// returns name of event type
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item)
{
  switch( item->flags.general.type ) {
  case MBNG_EVENT_TYPE_NOTE_OFF:       return "NoteOff";
  case MBNG_EVENT_TYPE_NOTE_ON:        return "NoteOn";
  case MBNG_EVENT_TYPE_POLY_PRESSURE:  return "PolyPressure";
  case MBNG_EVENT_TYPE_CC:             return "CC";
  case MBNG_EVENT_TYPE_PROGRAM_CHANGE: return "ProgramChange";
  case MBNG_EVENT_TYPE_AFTERTOUCH:     return "Aftertouch";
  case MBNG_EVENT_TYPE_PITCHBEND:      return "Pitchbend";
  case MBNG_EVENT_TYPE_SYSEX:          return "SysEx";
  case MBNG_EVENT_TYPE_NRPN:           return "NRPN";
  case MBNG_EVENT_TYPE_META:           return "Meta";
  }
  return "Disabled";
}

/////////////////////////////////////////////////////////////////////////////
// returns value of event type given as string
// returns <0 if invalid type
/////////////////////////////////////////////////////////////////////////////
mbng_event_type_t MBNG_EVENT_ItemTypeFromStrGet(char *event_type)
{
  if( strcasecmp(event_type, "NoteOff") == 0 )       return MBNG_EVENT_TYPE_NOTE_OFF;
  if( strcasecmp(event_type, "NoteOn") == 0 )        return MBNG_EVENT_TYPE_NOTE_ON;
  if( strcasecmp(event_type, "PolyPressure") == 0 )  return MBNG_EVENT_TYPE_POLY_PRESSURE;
  if( strcasecmp(event_type, "CC") == 0 )            return MBNG_EVENT_TYPE_CC;
  if( strcasecmp(event_type, "ProgramChange") == 0 ) return MBNG_EVENT_TYPE_PROGRAM_CHANGE;
  if( strcasecmp(event_type, "Aftertouch") == 0 )    return MBNG_EVENT_TYPE_AFTERTOUCH;
  if( strcasecmp(event_type, "Pitchbend") == 0 )     return MBNG_EVENT_TYPE_PITCHBEND;
  if( strcasecmp(event_type, "SysEx") == 0 )         return MBNG_EVENT_TYPE_SYSEX;
  if( strcasecmp(event_type, "NRPN") == 0 )          return MBNG_EVENT_TYPE_NRPN;
  if( strcasecmp(event_type, "Meta") == 0 )          return MBNG_EVENT_TYPE_META;

  return MBNG_EVENT_TYPE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for button mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemButtonModeStrGet(mbng_event_item_t *item)
{
  mbng_event_button_mode_t button_mode = item->flags.DIN.button_mode;
  switch( button_mode ) {
  case MBNG_EVENT_BUTTON_MODE_ON_OFF:  return "OnOff";
  case MBNG_EVENT_BUTTON_MODE_ON_ONLY: return "OnOnly";
  case MBNG_EVENT_BUTTON_MODE_TOGGLE:  return "Toggle";
  }
  return "Undefined";
}

mbng_event_button_mode_t MBNG_EVENT_ItemButtonModeFromStrGet(char *button_mode)
{
  if( strcasecmp(button_mode, "OnOff") == 0 )  return MBNG_EVENT_BUTTON_MODE_ON_OFF;
  if( strcasecmp(button_mode, "OnOnly") == 0 ) return MBNG_EVENT_BUTTON_MODE_ON_ONLY;
  if( strcasecmp(button_mode, "Toggle") == 0 ) return MBNG_EVENT_BUTTON_MODE_TOGGLE;

  return MBNG_EVENT_BUTTON_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for AIN Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemAinModeStrGet(mbng_event_item_t *item)
{
  mbng_event_ain_mode_t ain_mode = item->flags.AIN.ain_mode;
  switch( ain_mode ) {
  case MBNG_EVENT_AIN_MODE_DIRECT:                return "Direct";
  case MBNG_EVENT_AIN_MODE_SNAP:                  return "Snap";
  case MBNG_EVENT_AIN_MODE_RELATIVE:              return "Relative";
  case MBNG_EVENT_AIN_MODE_PARALLAX:              return "Parallax";
  }
  return "Undefined";
}

mbng_event_ain_mode_t MBNG_EVENT_ItemAinModeFromStrGet(char *ain_mode)
{
  if( strcasecmp(ain_mode, "Direct") == 0 )       return MBNG_EVENT_AIN_MODE_DIRECT;
  if( strcasecmp(ain_mode, "Snap") == 0 )         return MBNG_EVENT_AIN_MODE_SNAP;
  if( strcasecmp(ain_mode, "Relative") == 0 )     return MBNG_EVENT_AIN_MODE_RELATIVE;
  if( strcasecmp(ain_mode, "Parallax") == 0 )     return MBNG_EVENT_AIN_MODE_PARALLAX;

  return MBNG_EVENT_AIN_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for ENC Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemEncModeStrGet(mbng_event_item_t *item)
{
  mbng_event_enc_mode_t enc_mode = item->flags.ENC.enc_mode;
  switch( enc_mode ) {
  case MBNG_EVENT_ENC_MODE_ABSOLUTE:              return "Absolute";
  case MBNG_EVENT_ENC_MODE_40SPEED:               return "40Speed";
  case MBNG_EVENT_ENC_MODE_00SPEED:               return "00Speed";
  case MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED: return "Inc00Speed_Dec40Speed";
  case MBNG_EVENT_ENC_MODE_INC41_DEC3F:           return "Inc41_Dec3F";
  case MBNG_EVENT_ENC_MODE_INC01_DEC7F:           return "Inc01_Dec7F";
  case MBNG_EVENT_ENC_MODE_INC01_DEC41:           return "Inc01_Dec41";
  case MBNG_EVENT_ENC_MODE_INC_DEC:               return "IncDec";
  }
  return "Undefined";
}

mbng_event_enc_mode_t MBNG_EVENT_ItemEncModeFromStrGet(char *enc_mode)
{
  if( strcasecmp(enc_mode, "Absolute") == 0 )               return MBNG_EVENT_ENC_MODE_ABSOLUTE;
  if( strcasecmp(enc_mode, "40Speed") == 0 )                return MBNG_EVENT_ENC_MODE_40SPEED;
  if( strcasecmp(enc_mode, "00Speed") == 0 )                return MBNG_EVENT_ENC_MODE_00SPEED;
  if( strcasecmp(enc_mode, "Inc00Speed_Dec40Speed") == 0 )  return MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED;
  if( strcasecmp(enc_mode, "Inc41_Dec3F") == 0 )            return MBNG_EVENT_ENC_MODE_INC41_DEC3F;
  if( strcasecmp(enc_mode, "Inc01_Dec7F") == 0 )            return MBNG_EVENT_ENC_MODE_INC01_DEC7F;
  if( strcasecmp(enc_mode, "Inc01_Dec41") == 0 )            return MBNG_EVENT_ENC_MODE_INC01_DEC41;
  if( strcasecmp(enc_mode, "IncDec") == 0 )                 return MBNG_EVENT_ENC_MODE_INC_DEC;

  return MBNG_EVENT_ENC_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for ENC Speed Mode
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemEncSpeedModeStrGet(mbng_event_item_t *item)
{
  mbng_event_enc_speed_mode_t enc_speed_mode = item->flags.ENC.enc_speed_mode;
  switch( enc_speed_mode ) {
  case MBNG_EVENT_ENC_SPEED_MODE_AUTO:    return "Auto";
  case MBNG_EVENT_ENC_SPEED_MODE_SLOW:    return "Slow";
  case MBNG_EVENT_ENC_SPEED_MODE_NORMAL:  return "Normal";
  case MBNG_EVENT_ENC_SPEED_MODE_FAST:    return "Fast";
  }
  return "Undefined";
}

mbng_event_enc_speed_mode_t MBNG_EVENT_ItemEncSpeedModeFromStrGet(char *enc_speed_mode)
{
  if( strcasecmp(enc_speed_mode, "Auto") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_AUTO;
  if( strcasecmp(enc_speed_mode, "Slow") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_SLOW;
  if( strcasecmp(enc_speed_mode, "Normal") == 0 )   return MBNG_EVENT_ENC_SPEED_MODE_NORMAL;
  if( strcasecmp(enc_speed_mode, "Fast") == 0 )     return MBNG_EVENT_ENC_SPEED_MODE_FAST;

  return MBNG_EVENT_ENC_SPEED_MODE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for DOUT Matrix pattern selection
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemLedMatrixPatternStrGet(mbng_event_item_t *item)
{
  mbng_event_led_matrix_pattern_t led_matrix_pattern = item->flags.ENC.led_matrix_pattern;
  switch( led_matrix_pattern ) {
  case MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED: return "Undefined";
  case MBNG_EVENT_LED_MATRIX_PATTERN_1:         return "1";
  case MBNG_EVENT_LED_MATRIX_PATTERN_2:         return "2";
  case MBNG_EVENT_LED_MATRIX_PATTERN_3:         return "3";
  case MBNG_EVENT_LED_MATRIX_PATTERN_4:         return "4";
  case MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO:   return "LcAuto";
  }
  return "Undefined";
}

mbng_event_led_matrix_pattern_t MBNG_EVENT_ItemLedMatrixPatternFromStrGet(char *led_matrix_pattern)
{
  if( strcasecmp(led_matrix_pattern, "Undefined") == 0 ) return MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED;
  if( strcasecmp(led_matrix_pattern, "1") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_1;
  if( strcasecmp(led_matrix_pattern, "2") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_2;
  if( strcasecmp(led_matrix_pattern, "3") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_3;
  if( strcasecmp(led_matrix_pattern, "4") == 0 )         return MBNG_EVENT_LED_MATRIX_PATTERN_4;
  if( strcasecmp(led_matrix_pattern, "LcAuto") == 0 )    return MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO;

  return MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for (N)RPN format
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemNrpnFormatStrGet(mbng_event_item_t *item)
{
  mbng_event_nrpn_format_t nrpn_format = (item->stream_size >= 3) ? item->stream[2] : MBNG_EVENT_NRPN_FORMAT_UNSIGNED;
  switch( nrpn_format ) {
  case MBNG_EVENT_NRPN_FORMAT_SIGNED: return "Signed";
  }
  return "Unsigned";
}

mbng_event_nrpn_format_t MBNG_EVENT_ItemNrpnFormatFromStrGet(char *nrpn_format)
{
  if( strcasecmp(nrpn_format, "Unsigned") == 0 ) return MBNG_EVENT_NRPN_FORMAT_UNSIGNED;
  if( strcasecmp(nrpn_format, "Signed") == 0 ) return MBNG_EVENT_NRPN_FORMAT_SIGNED;

  return MBNG_EVENT_NRPN_FORMAT_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for SysEx variables
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemSysExVarStrGet(mbng_event_item_t *item, u8 stream_pos)
{
  mbng_event_sysex_var_t sysex_var = (item->stream_size >= stream_pos) ? item->stream[stream_pos] : MBNG_EVENT_SYSEX_VAR_UNDEFINED;
  switch( sysex_var ) {
  case MBNG_EVENT_SYSEX_VAR_DEV:        return "dev";
  case MBNG_EVENT_SYSEX_VAR_PAT:        return "pat";
  case MBNG_EVENT_SYSEX_VAR_BNK:        return "bnk";
  case MBNG_EVENT_SYSEX_VAR_INS:        return "ins";
  case MBNG_EVENT_SYSEX_VAR_CHN:        return "chn";
  case MBNG_EVENT_SYSEX_VAR_CHK_START:  return "chk_start";
  case MBNG_EVENT_SYSEX_VAR_CHK:        return "chk";
  case MBNG_EVENT_SYSEX_VAR_CHK_INV:    return "chk_inv";
  case MBNG_EVENT_SYSEX_VAR_VAL:        return "val";
  case MBNG_EVENT_SYSEX_VAR_VAL_H:      return "val_h";
  case MBNG_EVENT_SYSEX_VAR_VAL_N1:     return "val_n1";
  case MBNG_EVENT_SYSEX_VAR_VAL_N2:     return "val_n2";
  case MBNG_EVENT_SYSEX_VAR_VAL_N3:     return "val_n3";
  case MBNG_EVENT_SYSEX_VAR_VAL_N4:     return "val_n4";
  case MBNG_EVENT_SYSEX_VAR_IGNORE:     return "ignore";
  case MBNG_EVENT_SYSEX_VAR_DUMP:       return "dump";
  }
  return "undef";
}

mbng_event_sysex_var_t MBNG_EVENT_ItemSysExVarFromStrGet(char *sysex_var)
{
  if( strcasecmp(sysex_var, "dev") == 0 )        return MBNG_EVENT_SYSEX_VAR_DEV;
  if( strcasecmp(sysex_var, "pat") == 0 )        return MBNG_EVENT_SYSEX_VAR_PAT;
  if( strcasecmp(sysex_var, "bnk") == 0 )        return MBNG_EVENT_SYSEX_VAR_BNK;
  if( strcasecmp(sysex_var, "ins") == 0 )        return MBNG_EVENT_SYSEX_VAR_INS;
  if( strcasecmp(sysex_var, "chn") == 0 )        return MBNG_EVENT_SYSEX_VAR_CHN;
  if( strcasecmp(sysex_var, "chk_start") == 0 )  return MBNG_EVENT_SYSEX_VAR_CHK_START;
  if( strcasecmp(sysex_var, "chk") == 0 )        return MBNG_EVENT_SYSEX_VAR_CHK;
  if( strcasecmp(sysex_var, "chk_inv") == 0 )    return MBNG_EVENT_SYSEX_VAR_CHK_INV;
  if( strcasecmp(sysex_var, "val") == 0 )        return MBNG_EVENT_SYSEX_VAR_VAL;
  if( strcasecmp(sysex_var, "val_h") == 0 )      return MBNG_EVENT_SYSEX_VAR_VAL_H;
  if( strcasecmp(sysex_var, "val_n1") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N1;
  if( strcasecmp(sysex_var, "val_n2") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N2;
  if( strcasecmp(sysex_var, "val_n3") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N3;
  if( strcasecmp(sysex_var, "val_n4") == 0 )     return MBNG_EVENT_SYSEX_VAR_VAL_N4;
  if( strcasecmp(sysex_var, "ignore") == 0 )     return MBNG_EVENT_SYSEX_VAR_IGNORE;
  if( strcasecmp(sysex_var, "dump") == 0 )       return MBNG_EVENT_SYSEX_VAR_DUMP;
  return MBNG_EVENT_SYSEX_VAR_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// for Meta events
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_EVENT_ItemMetaTypeStrGet(mbng_event_item_t *item, u8 entry)
{
  mbng_event_meta_type_t meta_type = (item->stream_size >= (2*(entry+1))) ? item->stream[2*entry] : MBNG_EVENT_META_TYPE_UNDEFINED;
  switch( meta_type ) {
  case MBNG_EVENT_META_TYPE_SET_BANK:            return "SetBank";
  case MBNG_EVENT_META_TYPE_DEC_BANK:            return "DecBank";
  case MBNG_EVENT_META_TYPE_INC_BANK:            return "IncBank";

  case MBNG_EVENT_META_TYPE_ENC_FAST:            return "EncFast";

  case MBNG_EVENT_META_TYPE_SCS_ENC:             return "ScsEnc";
  case MBNG_EVENT_META_TYPE_SCS_MENU:            return "ScsMenu";
  case MBNG_EVENT_META_TYPE_SCS_SOFT1:           return "ScsSoft1";
  case MBNG_EVENT_META_TYPE_SCS_SOFT2:           return "ScsSoft2";
  case MBNG_EVENT_META_TYPE_SCS_SOFT3:           return "ScsSoft3";
  case MBNG_EVENT_META_TYPE_SCS_SOFT4:           return "ScsSoft4";
  case MBNG_EVENT_META_TYPE_SCS_SHIFT:           return "ScsShift";
  }

  return "Undefined";
}

mbng_event_meta_type_t MBNG_EVENT_ItemMetaTypeFromStrGet(char *meta_type)
{
  if( strcasecmp(meta_type, "SetBank") == 0 )       return MBNG_EVENT_META_TYPE_SET_BANK;
  if( strcasecmp(meta_type, "DecBank") == 0 )       return MBNG_EVENT_META_TYPE_DEC_BANK;
  if( strcasecmp(meta_type, "IncBank") == 0 )       return MBNG_EVENT_META_TYPE_INC_BANK;

  if( strcasecmp(meta_type, "EncFast") == 0 )       return MBNG_EVENT_META_TYPE_ENC_FAST;

  if( strcasecmp(meta_type, "ScsEnc") == 0 )        return MBNG_EVENT_META_TYPE_SCS_ENC;
  if( strcasecmp(meta_type, "ScsMenu") == 0 )       return MBNG_EVENT_META_TYPE_SCS_MENU;
  if( strcasecmp(meta_type, "ScsSoft1") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT1;
  if( strcasecmp(meta_type, "ScsSoft2") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT2;
  if( strcasecmp(meta_type, "ScsSoft3") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT3;
  if( strcasecmp(meta_type, "ScsSoft4") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SOFT4;
  if( strcasecmp(meta_type, "ScsShift") == 0 )      return MBNG_EVENT_META_TYPE_SCS_SHIFT;
  return MBNG_EVENT_META_TYPE_UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
// Sends the item via MIDI
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item)
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
      p.evnt1 = item->value & 0x7f;
      p.evnt2 = 0;
      break;

    case MBNG_EVENT_TYPE_PITCHBEND:
      if( (item->max-item->min) < 128 ) { // 7bit range
	u8 value = item->value & 0x7f; // just to ensure
	p.evnt1 = (value == 0x40) ? 0x00 : value;
	p.evnt2 = value;
      } else {
	p.evnt1 = (item->value & 0x7f);
	p.evnt2 = (item->value >> 7) & 0x7f;
      }
      break;

    default:
      if( item->flags.general.use_key_or_cc ) { // key=any or cc=any
	p.evnt1 = item->value & 0x7f;
	p.evnt2 = item->secondary_value;
      } else {
	p.evnt1 = item->stream[1] & 0x7f;
	p.evnt2 = item->value & 0x7f;
      }
    }

    // EXTRA for BUTTON_MATRIX: modify MIDI event depending on pin number
    // NOTE: if dedicated MIDI events should be sent, use the button_emu_id_offset feature!
    if( item->matrix_pin && event_type < MBNG_EVENT_TYPE_CC ) {
      int new = p.evnt1 + item->matrix_pin;
      if( new >= 127 )
	new = 127;
      p.evnt1 = new;
    }

    // send MIDI package over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	MUTEX_MIDIOUT_TAKE;
	MIOS32_MIDI_SendPackage(port, p);
	MUTEX_MIDIOUT_GIVE;
      }
    }
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {

    // create MIDI package
    mios32_midi_package_t p;
    p.ALL = 0;
    p.type = CC;
    p.event = CC;

    if( mbng_patch_cfg.global_chn )
      p.chn = mbng_patch_cfg.global_chn - 1;
    else
      p.chn = item->stream[0] & 0xf;

    u16 nrpn_address = item->stream[1] | ((u16)item->stream[2] << 7);
    mbng_event_nrpn_format_t nrpn_format = item->stream[3]; // TODO
    u8 nrpn_address_msb = (nrpn_address >> 7) & 0x7f;
    u8 nrpn_address_lsb = (nrpn_address >> 0) & 0x7f;
    u16 nrpn_value = item->value;
    u8 nrpn_value_msb = (nrpn_value >> 7) & 0x7f;
    u8 nrpn_value_lsb = (nrpn_value >> 0) & 0x7f;

    // send optimized NRPNs over enabled ports
    int i;
    u16 mask = 1;
    for(i=0; i<16; ++i, mask <<= 1) {
      if( item->enabled_ports & mask ) {
	// USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);

	if( mask & MBNG_EVENT_NRPN_SEND_PORTS_MASK ) {
	  u8 port_ix = i - MBNG_EVENT_NRPN_SEND_PORTS_OFFSET;

	  MUTEX_MIDIOUT_TAKE;
	  if( (nrpn_address ^ nrpn_sent_address[port_ix][p.chn]) >> 7 ) { // new MSB - will also cover the case that nrpn_sent_address == 0xffff
	    p.cc_number = 0x63; // Address MSB
	    p.value = nrpn_address_msb;
	    MIOS32_MIDI_SendPackage(port, p);
	  }

	  if( (nrpn_address ^ nrpn_sent_address[port_ix][p.chn]) & 0x7f ) { // new LSB
	    p.cc_number = 0x62; // Address LSB
	    p.value = nrpn_address_lsb;
	    MIOS32_MIDI_SendPackage(port, p);
	  }

	  if( (nrpn_value ^ nrpn_sent_value[port_ix][p.chn]) >> 7 ) { // new MSB - will also cover the case that nrpn_sent_value == 0xffff
	    p.cc_number = 0x06; // Data MSB
	    p.value = nrpn_value_msb;
	    MIOS32_MIDI_SendPackage(port, p);
	  }

	  if( (nrpn_value ^ nrpn_sent_value[port_ix][p.chn]) & 0x7f ) { // new LSB
	    p.cc_number = 0x26; // Data LSB
	    p.value = nrpn_value_lsb;
	    MIOS32_MIDI_SendPackage(port, p);
	  }
	  MUTEX_MIDIOUT_GIVE;

	  nrpn_sent_address[port_ix][p.chn] = nrpn_address;
	  nrpn_sent_value[port_ix][p.chn] = nrpn_value;

	} else {
	  MUTEX_MIDIOUT_TAKE;

	  p.cc_number = 0x63; // Address MSB
	  p.value = nrpn_address_msb;
	  MIOS32_MIDI_SendPackage(port, p);

	  p.cc_number = 0x62; // Address LSB
	  p.value = nrpn_address_lsb;
	  MIOS32_MIDI_SendPackage(port, p);

	  p.cc_number = 0x06; // Data MSB
	  p.value = nrpn_value_msb;
	  MIOS32_MIDI_SendPackage(port, p);

	  p.cc_number = 0x26; // Data LSB
	  p.value = nrpn_value_lsb;
	  MIOS32_MIDI_SendPackage(port, p);

	  MUTEX_MIDIOUT_GIVE;
	}
      }
    }
    return 0; // no error

  } else if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
#define STREAM_MAX_SIZE 128
    u8 stream[STREAM_MAX_SIZE]; // note: it's ensure that the out stream isn't longer than the in stream, therefore no size checks required
    u8 *stream_out = stream;
    u8 *stream_in = item->stream;
    u8 *stream_in_end = (u8 *)(item->stream + item->stream_size - 1);
    u8 chk = 0;
    while( stream_in <= stream_in_end ) {
      u8 new_value = 1;

      if( *stream_in == 0xff ) {
	++stream_in;
	switch( *stream_in++ ) {
	case MBNG_EVENT_SYSEX_VAR_DEV:        *stream_out = mbng_patch_cfg.sysex_dev; break;
	case MBNG_EVENT_SYSEX_VAR_PAT:        *stream_out = mbng_patch_cfg.sysex_pat; break;
	case MBNG_EVENT_SYSEX_VAR_BNK:        *stream_out = mbng_patch_cfg.sysex_bnk; break;
	case MBNG_EVENT_SYSEX_VAR_INS:        *stream_out = mbng_patch_cfg.sysex_ins; break;
	case MBNG_EVENT_SYSEX_VAR_CHN:        *stream_out = mbng_patch_cfg.sysex_chn; break;
	case MBNG_EVENT_SYSEX_VAR_CHK_START:  new_value = 0; chk = 0; break;
	case MBNG_EVENT_SYSEX_VAR_CHK:        *stream_out = chk & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_CHK_INV:    *stream_out = (chk ^ 0x7f) & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_VAL:        *stream_out = item->value & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_H:      *stream_out = (item->value >>  7) & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N1:     *stream_out = (item->value >>  0) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N2:     *stream_out = (item->value >>  4) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N3:     *stream_out = (item->value >>  8) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N4:     *stream_out = (item->value >> 12) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_IGNORE:     new_value = 0; break;
	case MBNG_EVENT_SYSEX_VAR_DUMP:       new_value = 0; break; // not relevant for transmitter (yet) - or should we allow to send a dump stored dump?
	default: new_value = 0;
	}
      } else {
	new_value = 1;
	*stream_out = *stream_in++;
      }

      if( new_value )
	chk += *stream_out++;
    }

    // send SysEx over enabled ports
    {
      int len = stream_out - stream;
      if( len >= 1 ) {
	int i;
	u16 mask = 1;
	for(i=0; i<16; ++i, mask <<= 1) {
	  if( item->enabled_ports & mask ) {
	    // USB0/1/2/3, UART0/1/2/3, IIC0/1/2/3, OSC0/1/2/3
	    mios32_midi_port_t port = USB0 + ((i&0xc) << 2) + (i&3);
	    MUTEX_MIDIOUT_TAKE;
	    MIOS32_MIDI_SendSysEx(port, stream, len);
	    MUTEX_MIDIOUT_GIVE;
	  }
	}
      }
    }
    return 0;
  } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_META ) {
    int i;
    for(i=0; i<item->stream_size; i+=2) {
      mbng_event_meta_type_t meta_type = item->stream[i+0];
      u8 meta_value = item->stream[i+1];
      switch( meta_type ) {
      case MBNG_EVENT_META_TYPE_SET_BANK: {
	if( item->value ) {
	  MBNG_PATCH_BankSet(item->value-1); // user counts from 1, internally we are counting from 0
	}
      } break;

      case MBNG_EVENT_META_TYPE_DEC_BANK: {
	s32 bank = MBNG_PATCH_BankGet() + 1; // user counts from 1, internally we are counting from 0
	if( bank > 1 )
	  MBNG_PATCH_BankSet(bank-1 - 1);
      } break;

      case MBNG_EVENT_META_TYPE_INC_BANK: {
	s32 bank = MBNG_PATCH_BankGet() + 1; // user counts from 1, internally we are counting from 0
	if( bank < MBNG_PATCH_NumBanksGet() )
	  MBNG_PATCH_BankSet(bank-1 + 1);
      } break;


      case MBNG_EVENT_META_TYPE_ENC_FAST: {
	MBNG_ENC_FastModeSet(item->value);
      } break;

      case MBNG_EVENT_META_TYPE_SCS_ENC: {
	SCS_ENC_MENU_NotifyChange((s32)(item->value - 64));
      } break;
      case MBNG_EVENT_META_TYPE_SCS_MENU: {
	SCS_DIN_NotifyToggle(SCS_PIN_EXIT, item->value == 0);
      } break;
      case MBNG_EVENT_META_TYPE_SCS_SOFT1: {
	SCS_DIN_NotifyToggle(SCS_PIN_SOFT1, item->value == 0);
      } break;
      case MBNG_EVENT_META_TYPE_SCS_SOFT2: {
	SCS_DIN_NotifyToggle(SCS_PIN_SOFT2, item->value == 0);
      } break;
      case MBNG_EVENT_META_TYPE_SCS_SOFT3: {
	SCS_DIN_NotifyToggle(SCS_PIN_SOFT3, item->value == 0);
      } break;
      case MBNG_EVENT_META_TYPE_SCS_SOFT4: {
	SCS_DIN_NotifyToggle(SCS_PIN_SOFT4, item->value == 0);
      } break;
      case MBNG_EVENT_META_TYPE_SCS_SHIFT: {
	SCS_DIN_NotifyToggle(SCS_PIN_SOFT5, item->value == 0);
      } break;
      }
    }
  } else {
    return -1; // unsupported
  }

  return -1; // unsupported item type
}


/////////////////////////////////////////////////////////////////////////////
// Called when an item should be notified
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value, u8 from_midi)
{
  // take over new value
  item->value = value;

  // value modified from MIDI?
  item->flags.general.value_from_midi = from_midi;

  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->value = item->value;
    pool_item->flags.general.value_from_midi = from_midi;
  }

  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_DEBUG ) {
    MBNG_EVENT_ItemPrint(item);
  }

  switch( item->id & 0xf000 ) {
  //case MBNG_EVENT_CONTROLLER_DISABLED:
  case MBNG_EVENT_CONTROLLER_BUTTON:        MBNG_DIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_LED:           MBNG_DOUT_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: MBNG_MATRIX_DIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    MBNG_MATRIX_DOUT_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_ENC:           MBNG_ENC_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_AIN:           MBNG_AIN_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_AINSER:        MBNG_AINSER_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_MF:            MBNG_MF_NotifyReceivedValue(item); break;
  case MBNG_EVENT_CONTROLLER_CV:            MBNG_CV_NotifyReceivedValue(item); break;

  case MBNG_EVENT_CONTROLLER_SENDER: {
    int sender_ix = item->id & 0xfff;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("MBNG_EVENT_ItemReceive(%d, %d) (Sender)\n", sender_ix, item->value);
    }

    // map?
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
    if( map_len > 0 ) {
      item->value = map_values[(value < map_len) ? value : (map_len-1)];
    }

    // send MIDI event
    MBNG_EVENT_ItemSend(item);
  } break;

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
    int receiver_ix = item->id & 0xfff;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("MBNG_EVENT_ItemReceive(%d, %d) (Receiver)\n", receiver_ix, item->value);
    }

    // map?
    u8 *map_values;
    int map_len = MBNG_EVENT_MapGet(item->map, &map_values);
    if( map_len > 0 ) {
      item->value = map_values[(value < map_len) ? value : (map_len-1)];
    }
  } break;
  }

  // forward
  if( !MBNG_PATCH_BankCtrlInBank(item) || MBNG_PATCH_BankCtrlIsActive(item) ) {
    if( item->fwd_id )
      MBNG_EVENT_ItemForward(item);
    else {
      u8 radio_group = 0;
      switch( item->id & 0xf000 ) {
      case MBNG_EVENT_CONTROLLER_BUTTON: radio_group = item->flags.DIN.radio_group; break;
      case MBNG_EVENT_CONTROLLER_LED:    radio_group = item->flags.DOUT.radio_group; break;
      }

      if( radio_group )
	MBNG_EVENT_ItemForwardToRadioGroup(item, radio_group);
    }

    if( item->flags.general.fwd_to_lcd )
      MBNG_LCD_PrintItemLabel(item);
  }

  return -1; // unsupported controller type
}


/////////////////////////////////////////////////////////////////////////////
// Called to forward an event
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemForward(mbng_event_item_t *item)
{
  static u8 recursion_ctr = 0;

  if( !item->fwd_id )
    return -1; // no forwarding enabled

  if( item->fwd_id == item->id )
    return -2; // avoid feedback

  if( recursion_ctr >= MBNG_EVENT_MAX_FWD_RECURSION )
    return -3;
  ++recursion_ctr;

  // search for fwd item
  mbng_event_item_t fwd_item;
  if( MBNG_EVENT_ItemSearchById(item->fwd_id, &fwd_item) >= 0 ) {
    // take over value of item in pool item
    if( fwd_item.pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
      mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + fwd_item.pool_address);
      pool_item->value = item->value;
      pool_item->flags.general.value_from_midi = item->flags.general.value_from_midi;
    }

    // notify item
    MBNG_EVENT_ItemReceive(&fwd_item, item->value, 0);
  } else {
    // notify by temporary changing the ID - forwarding disabled
    mbng_event_item_id_t tmp_id = item->id;
    mbng_event_item_id_t tmp_fwd_id = item->fwd_id;
    u16 tmp_pool_address = item->pool_address;
    u8 tmp_fwd_to_lcd = item->flags.general.fwd_to_lcd;
    item->id = item->fwd_id;
    item->fwd_id = 0;
    item->pool_address = 0xffff;
    item->flags.general.fwd_to_lcd = 0;
    MBNG_EVENT_ItemReceive(item, item->value, 0);
    item->id = tmp_id;
    item->fwd_id = tmp_fwd_id;
    item->pool_address = tmp_pool_address;
    item->flags.general.fwd_to_lcd = tmp_fwd_to_lcd;
  }

  if( recursion_ctr )
    --recursion_ctr;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Called to forward an event to a radio group
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ItemForwardToRadioGroup(mbng_event_item_t *item, u8 radio_group)
{
  // search for all items in the pool which are part of the radio group
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    u16 id_type = pool_item->id & 0xf000;
    u8 is_in_group = 0;
    if( id_type == MBNG_EVENT_CONTROLLER_BUTTON ) {
      is_in_group = pool_item->flags.DIN.radio_group == radio_group;
    }
    else if( id_type == MBNG_EVENT_CONTROLLER_LED ) {
      is_in_group = pool_item->flags.DOUT.radio_group == radio_group;
    }

    if( is_in_group ) {
      mbng_event_item_t fwd_item;
      MBNG_EVENT_ItemCopy2User(pool_item, &fwd_item);
      fwd_item.value = item->value; // forward the value of the sender

      // take over value of item in pool item
      if( fwd_item.pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
	mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + fwd_item.pool_address);
	pool_item->value = item->value;
	pool_item->flags.general.value_from_midi = item->flags.general.value_from_midi;
      }

      // update value in button/LED element
      if( id_type == MBNG_EVENT_CONTROLLER_BUTTON )
	MBNG_DIN_NotifyReceivedValue(&fwd_item);
      else if( id_type == MBNG_EVENT_CONTROLLER_LED )
	MBNG_DOUT_NotifyReceivedValue(&fwd_item);

      // and trigger forwarding
      MBNG_EVENT_ItemForward(&fwd_item);
    }

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called from a controller to notify that a value should be sent
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_NotifySendValue(mbng_event_item_t *item)
{
  // value not modified from MIDI anymore
  item->flags.general.value_from_midi = 0;

  // take over in pool item
  if( item->pool_address < MBNG_EVENT_POOL_MAX_SIZE ) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)((u32)&event_pool[0] + item->pool_address);
    pool_item->value = item->value;
    pool_item->flags.general.value_from_midi = 0;
  }

  // send MIDI event
  MBNG_EVENT_ItemSend(item);

  // forward - optionally to whole radio group
  u8 radio_group = 0;
  switch( item->id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_BUTTON: radio_group = item->flags.DIN.radio_group; break;
  case MBNG_EVENT_CONTROLLER_LED:    radio_group = item->flags.DOUT.radio_group; break;
  }

  if( radio_group )
    MBNG_EVENT_ItemForwardToRadioGroup(item, radio_group);
  else
    MBNG_EVENT_ItemForward(item);

  // print label
  MBNG_LCD_PrintItemLabel(item);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Refresh all items
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_Refresh(void)
{
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;

    mbng_event_item_t item;
    MBNG_EVENT_ItemCopy2User(pool_item, &item);
    item.flags.general.fwd_to_lcd = 1; // force LCD update
    MBNG_EVENT_ItemReceive(&item, pool_item->value, 1);

    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called from MBNG_EVENT_ReceiveSysEx() when a Syxdump is received
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_EVENT_NotifySyxDump(u8 from_receiver, u16 dump_pos, u8 value)
{
  // search in pool for events which listen to this receiver and dump_pos
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    if( pool_item->syxdump_pos.pos == dump_pos &&
	pool_item->syxdump_pos.receiver == from_receiver ) {

      mbng_event_item_t item;
      MBNG_EVENT_ItemCopy2User(pool_item, &item);
      MBNG_EVENT_ItemReceive(&item, value, 1);
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_MIDI_NotifyPackage whenver a new
// MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // create port mask, and check if this is a supported port (USB0..3, UART0..3, IIC0..3, OSC0..3)
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0x30)>>2;
  u16 port_mask = subport_mask << port_class;
  if( !port_mask )
    return -1; // not supported

  // Note Off -> Note On with velocity 0
  if( mbng_patch_cfg.convert_note_off_to_on0 ) {
    if( midi_package.event == NoteOff ) {
      midi_package.event = NoteOn;
      midi_package.velocity = 0;
    }
  }

  // on CC:
  u16 nrpn_address = 0xffff; // taken if < 0xffff
  u16 nrpn_value = 0;
  if( midi_package.event == CC ) {

    // track NRPN event
    if( port_mask & MBNG_EVENT_NRPN_RECEIVE_PORTS_MASK ) {
      int port_ix = (port_class | (port & 3)) - MBNG_EVENT_NRPN_RECEIVE_PORTS_OFFSET;
      if( port_ix >= 0 && port_ix < MBNG_EVENT_NRPN_RECEIVE_PORTS ) {
        // NRPN handling
        switch( midi_package.cc_number ) {
        case 0x63: { // Address MSB
	  nrpn_received_address[port_ix][midi_package.chn] &= ~0x3f80;
	  nrpn_received_address[port_ix][midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);
        } break;

        case 0x62: { // Address LSB
	  nrpn_received_address[port_ix][midi_package.chn] &= ~0x007f;
	  nrpn_received_address[port_ix][midi_package.chn] |= (midi_package.value & 0x007f);
        } break;

        case 0x06: { // Data MSB
	  nrpn_received_value[port_ix][midi_package.chn] &= ~0x3f80;
	  nrpn_received_value[port_ix][midi_package.chn] |= ((midi_package.value << 7) & 0x3f80);
	  // nrpn_value = nrpn_received_value[port_ix][midi_package.chn]; // pass to parser
	  // MEMO: it's better to update only when LSB has been received
	  nrpn_address = nrpn_received_address[port_ix][midi_package.chn];
        } break;

        case 0x26: { // Data LSB
	  nrpn_received_value[port_ix][midi_package.chn] &= ~0x007f;
	  nrpn_received_value[port_ix][midi_package.chn] |= (midi_package.value & 0x007f);
	  nrpn_value = nrpn_received_value[port_ix][midi_package.chn]; // pass to parser
	  nrpn_address = nrpn_received_address[port_ix][midi_package.chn];
        } break;
        }
      }
    }

    // check for "all notes off" command
    if( mbng_patch_cfg.all_notes_off_chn &&
	(midi_package.chn == (mbng_patch_cfg.all_notes_off_chn - 1)) &&
	midi_package.cc_number == 123 ) {
      MBNG_DOUT_Init(0);
    }
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
      
      if( (pool_item->id & 0xf000) == MBNG_EVENT_CONTROLLER_SENDER ) { // a sender doesn't receive
	pool_ptr += pool_item->len;
	continue;
      }

      if( !(pool_item->enabled_ports & port_mask) ) { // port not enabled
	pool_ptr += pool_item->len;
	continue;
      }

      mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).general.type;
      if( event_type <= MBNG_EVENT_TYPE_CC ) {
	u8 *stream = &pool_item->data_begin;
	if( stream[1] >= 128 || evnt1 == stream[1] ) {
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  item.secondary_value = midi_package.value;
	  if( item.flags.general.use_key_or_cc ) {
	    MBNG_EVENT_ItemReceive(&item, midi_package.evnt1, 1);
	  } else {
	    MBNG_EVENT_ItemReceive(&item, midi_package.value, 1);
	  }
	} else {
	  // EXTRA for button/led matrices
	  int matrix = (pool_item->id & 0x0fff) - 1;
	  int num_pins = -1;

	  switch( pool_item->id & 0xf000 ) {
	  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
	    if( matrix >= 0 && matrix < MBNG_PATCH_NUM_MATRIX_DIN ) {
	      mbng_patch_matrix_din_entry_t *m = (mbng_patch_matrix_din_entry_t *)&mbng_patch_matrix_din[matrix];

	      if( m->button_emu_id_offset && m->sr_din1 ) {
		num_pins = 8 * m->num_rows;
		if( m->sr_din2 )
		  num_pins *= 2;
	      }
	    }
	  } break;
	  case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
	    if( matrix >= 0 && matrix < MBNG_PATCH_NUM_MATRIX_DOUT ) {
	      mbng_patch_matrix_dout_entry_t *m = (mbng_patch_matrix_dout_entry_t *)&mbng_patch_matrix_dout[matrix];

	      if( m->led_emu_id_offset && m->sr_dout_r1 ) {
		num_pins = 8 * m->num_rows;
		if( m->sr_dout_r2 )
		  num_pins *= 2;
	      }
	    }
	  } break;
	  }

	  if( num_pins >= 0 ) {
	    int first_evnt1 = stream[1];
	    if( evnt1 >= first_evnt1 && evnt1 < (first_evnt1 + num_pins) ) {
	      mbng_event_item_t item;
	      MBNG_EVENT_ItemCopy2User(pool_item, &item);
	      item.matrix_pin = evnt1 - first_evnt1;
	      MBNG_EVENT_ItemReceive(&item, midi_package.value, 1);
	    }
	  }
	}
      } else if( event_type <= MBNG_EVENT_TYPE_AFTERTOUCH ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1, 1);
      } else if( event_type == MBNG_EVENT_TYPE_PITCHBEND ) {
	mbng_event_item_t item;
	MBNG_EVENT_ItemCopy2User(pool_item, &item);
	MBNG_EVENT_ItemReceive(&item, evnt1 | ((u16)midi_package.value << 7), 1);
      } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {
	u8 *stream = &pool_item->data_begin;
	u16 expected_address = stream[1] | ((u16)stream[2] << 7);
	if( nrpn_address == expected_address ) {
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  MBNG_EVENT_ItemReceive(&item, nrpn_value, 1);
	}
      } else {
	// no additional event types yet...
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_SYSEX_Parser on incoming SysEx data
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  // create port mask, and check if this is a supported port (USB0..3, UART0..3, IIC0..3, OSC0..3)
  u8 subport_mask = (1 << (port&3));
  u8 port_class = ((port-0x10) & 0x30)>>2;
  u16 port_mask = subport_mask << port_class;
  if( !port_mask )
    return -1; // not supported

  // search in pool for matching SysEx streams
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).general.type;
    if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
      u8 parse_sysex = 1;

      if( (pool_item->id & 0xf000) == MBNG_EVENT_CONTROLLER_SENDER ) // a sender doesn't receive
	parse_sysex = 0;

      if( !(pool_item->enabled_ports & port_mask) ) // port not enabled
	parse_sysex = 0;

      // receiving a SysEx dump?
      if( pool_item->tmp_runtime_flags.sysex_dump ) {
	if( midi_in >= 0xf0 ) {
	  // notify event when all values have been received
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  MBNG_EVENT_ItemReceive(&item, pool_item->tmp_sysex_match_ctr, 1); // passing byte counter as value, it could be interesting

	  // and reset
	  pool_item->tmp_runtime_flags.sysex_dump = 0; // finished
	  pool_item->tmp_sysex_match_ctr = 0;
	} else {
	  // notify all events which listen to this dump
	  MBNG_EVENT_NotifySyxDump(pool_item->id & 0xff, pool_item->tmp_sysex_match_ctr, midi_in);

	  // waiting for next byte
	  ++pool_item->tmp_sysex_match_ctr;
	  parse_sysex = 0;
	}
      }

      if( parse_sysex ) {
	u8 *stream = ((u8 *)&pool_item->data_begin) + pool_item->tmp_sysex_match_ctr;
	u8 again = 0;
	do {
	  if( *stream == 0xff ) { // SysEx variable
	    u8 match = 0;
	    switch( *++stream ) {
	    case MBNG_EVENT_SYSEX_VAR_DEV:        match = midi_in == mbng_patch_cfg.sysex_dev; break;
	    case MBNG_EVENT_SYSEX_VAR_PAT:        match = midi_in == mbng_patch_cfg.sysex_pat; break;
	    case MBNG_EVENT_SYSEX_VAR_BNK:        match = midi_in == mbng_patch_cfg.sysex_bnk; break;
	    case MBNG_EVENT_SYSEX_VAR_INS:        match = midi_in == mbng_patch_cfg.sysex_ins; break;
	    case MBNG_EVENT_SYSEX_VAR_CHN:        match = midi_in == mbng_patch_cfg.sysex_chn; break;
	    case MBNG_EVENT_SYSEX_VAR_CHK_START:  match = 1; again = 1; break;
	    case MBNG_EVENT_SYSEX_VAR_CHK:        match = 1; break; // ignore checksum
	    case MBNG_EVENT_SYSEX_VAR_CHK_INV:    match = 1; break; // ignore checksum
	    case MBNG_EVENT_SYSEX_VAR_VAL:        match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0xff80) | (midi_in & 0x7f); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_H:      match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0xf07f) | ((midi_in & 0x7f) << 7); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N1:     match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0xfff0) | ((midi_in >>  0) & 0xf); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N2:     match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0xff0f) | ((midi_in >>  4) & 0xf); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N3:     match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0xf0ff) | ((midi_in >>  8) & 0xf); break;
	    case MBNG_EVENT_SYSEX_VAR_VAL_N4:     match = 1; pool_item->tmp_sysex_value = (pool_item->tmp_sysex_value & 0x0fff) | ((midi_in >> 12) & 0xf); break;
	    case MBNG_EVENT_SYSEX_VAR_IGNORE:     match = 1; break;
	    case MBNG_EVENT_SYSEX_VAR_DUMP:       match = 1; pool_item->tmp_runtime_flags.sysex_dump = 1; pool_item->tmp_sysex_match_ctr = 0; break; // enable dump receiver
	    }
	    if( match ) {
	      if( !pool_item->tmp_runtime_flags.sysex_dump ) {
		pool_item->tmp_sysex_match_ctr += 2;
	      }
	    } else {
	      pool_item->tmp_sysex_match_ctr = 0;
	      pool_item->tmp_runtime_flags.sysex_dump = 0;
	    }
	  } else if( *stream == midi_in ) { // matching byte
	    // begin of stream?
	    if( midi_in == 0xf0 ) {
	      pool_item->tmp_sysex_match_ctr = 0;
	      pool_item->tmp_sysex_value = 0;
	      pool_item->tmp_runtime_flags.sysex_dump = 0;
	    }

	    // end of stream?
	    if( midi_in == 0xf7 ) {
	      pool_item->tmp_sysex_match_ctr = 0;
	      pool_item->tmp_runtime_flags.sysex_dump = 0;

	      // all values matching!
	      mbng_event_item_t item;
	      MBNG_EVENT_ItemCopy2User(pool_item, &item);
	      MBNG_EVENT_ItemReceive(&item, pool_item->tmp_sysex_value, 1);
	    } else {
	      ++pool_item->tmp_sysex_match_ctr;
	    }
	  } else { // no matching byte
	    pool_item->tmp_sysex_match_ctr = 0;
	  }
	} while( again );
      }
    }
    pool_ptr += pool_item->len;
  }

  return 0; // no error
}
