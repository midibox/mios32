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
#include "mbng_lcd.h"
#include "mbng_din.h"
#include "mbng_dout.h"
#include "mbng_matrix.h"
#include "mbng_enc.h"
#include "mbng_ain.h"
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
  u16 flags; // (mbng_event_flags_t)
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
  u8 lcd;
  u8 lcd_pos;
  u8 data_begin; // data section for streams and label start here, it can have multiple bytes
} mbng_event_pool_item_t;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

#define MBNG_EVENT_POOL_MAX_SIZE 16384
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

  // Default Items:
  int i;

  // Buttons
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_BUTTON + i);
    item.flags.general.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_btn"); // Button #%3i %3d%b
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }
  item.flags.DIN.inverse = 0;

  // LEDs
  item.lcd_pos = 0x40;
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_LED + i);
    item.flags.general.type = MBNG_EVENT_TYPE_NOTE_ON;
    stream[0] = 0x90;
    stream[1] = 0x24 + i - 1;
    item.stream = stream;
    item.stream_size = 2;

    strcpy(str, "^std_led"); // LED #%3i       %b
    item.label = str;

    MBNG_EVENT_ItemAdd(&item);
  }
  item.lcd_pos = 0x00;

  // Encoders
  for(i=1; i<=64; ++i) {
    char str[21];
    u8 stream[20];

    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_ENC + i);
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
s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item, mbng_event_item_id_t id)
{
  item->id = id;
  item->flags.ALL = 0;
  item->enabled_ports = 0x1011; // OSC1, UART1 and USB1
  item->min    = 0;
  item->max    = 127;
  item->offset = 0;
  item->syxdump_pos.ALL = 0;
  item->stream_size = 0;
  item->lcd = 0;
  item->lcd_pos = 0x00;
  item->stream = NULL;
  item->label = NULL;

  // differ between type
  switch( id & 0xf000 ) {
  case MBNG_EVENT_CONTROLLER_BUTTON: {
    item->flags.DIN.button_mode = MBNG_EVENT_BUTTON_MODE_ON_OFF;
    item->flags.DIN.inverse = 1;
  }; break;

  case MBNG_EVENT_CONTROLLER_LED: {
  }; break;

  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: {
  }; break;

  case MBNG_EVENT_CONTROLLER_LED_MATRIX: {
  }; break;

  case MBNG_EVENT_CONTROLLER_ENC: {
    item->flags.ENC.enc_mode = MBNG_EVENT_ENC_MODE_ABSOLUTE;
  }; break;

  case MBNG_EVENT_CONTROLLER_AIN: {
  }; break;

  case MBNG_EVENT_CONTROLLER_AINSER: {
  }; break;

  case MBNG_EVENT_CONTROLLER_MF: {
  }; break;

  case MBNG_EVENT_CONTROLLER_CV: {
  }; break;

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
  }; break;
  }
  
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
  item->offset = pool_item->offset;
  item->syxdump_pos.ALL = pool_item->syxdump_pos.ALL;
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
  pool_item->flags = item->flags.ALL;
  pool_item->enabled_ports = item->enabled_ports;
  pool_item->min = item->min;
  pool_item->max = item->max;
  pool_item->offset = item->offset;
  pool_item->syxdump_pos.ALL = item->syxdump_pos.ALL;
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
#if 0
  MIOS32_MIDI_SendDebugMessage("[EVENT:%04x] %s %s stream:",
			       item->id,
			       MBNG_EVENT_ItemControllerStrGet(item),
			       MBNG_EVENT_ItemTypeStrGet(item));
  if( item->stream_size ) {
    MIOS32_MIDI_SendDebugHexDump(item->stream, item->stream_size);
  }
  return 0;
#else
  return MIOS32_MIDI_SendDebugMessage("[EVENT:%04x] %s %s ports:%04x min:%d max:%d label:%s\n",
				      item->id,
				      MBNG_EVENT_ItemControllerStrGet(item),
				      MBNG_EVENT_ItemTypeStrGet(item),
				      item->enabled_ports,
				      item->min,
				      item->max,
				      item->label ? item->label : "");
#endif
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
  case MBNG_EVENT_CONTROLLER_RECEIVER:      return "RECEIVER";
  }
  return "DISABLED";
}

/////////////////////////////////////////////////////////////////////////////
// returns ID of controller given as string
/////////////////////////////////////////////////////////////////////////////
mbng_event_item_id_t MBNG_EVENT_ItemIdFromControllerStrGet(char *event)
{
  if( strcasecmp(event, "BUTTON") == 0 )        return MBNG_EVENT_CONTROLLER_BUTTON;
  if( strcasecmp(event, "LED") == 0 )           return MBNG_EVENT_CONTROLLER_LED;
  if( strcasecmp(event, "BUTTON_MATRIX") == 0 ) return MBNG_EVENT_CONTROLLER_BUTTON_MATRIX;
  if( strcasecmp(event, "LED_MATRIX") == 0 )    return MBNG_EVENT_CONTROLLER_LED_MATRIX;
  if( strcasecmp(event, "ENC") == 0 )           return MBNG_EVENT_CONTROLLER_ENC;
  if( strcasecmp(event, "AIN") == 0 )           return MBNG_EVENT_CONTROLLER_AIN;
  if( strcasecmp(event, "AINSER") == 0 )        return MBNG_EVENT_CONTROLLER_AINSER;
  if( strcasecmp(event, "MF") == 0 )            return MBNG_EVENT_CONTROLLER_MF;
  if( strcasecmp(event, "CV") == 0 )            return MBNG_EVENT_CONTROLLER_CV;
  if( strcasecmp(event, "RECEIVER") == 0 )      return MBNG_EVENT_CONTROLLER_RECEIVER;

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
  case MBNG_EVENT_TYPE_RPN:            return "RPN";
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
  if( strcasecmp(event_type, "RPN") == 0 )           return MBNG_EVENT_TYPE_RPN;
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
  case MBNG_EVENT_META_TYPE_SET_BUTTON_GROUP:      return "SetButtonGroup";
  case MBNG_EVENT_META_TYPE_SET_LED_GROUP:         return "SetLedGroup";
  case MBNG_EVENT_META_TYPE_SET_ENC_GROUP:         return "SetEncGroup";
  case MBNG_EVENT_META_TYPE_SET_DIN_MATRIX_GROUP:  return "SetDinMatrixGroup";
  case MBNG_EVENT_META_TYPE_SET_DOUT_MATRIX_GROUP: return "SetDoutMatrixGroup";
  case MBNG_EVENT_META_TYPE_SET_AIN_GROUP:         return "SetAinGroup";
  case MBNG_EVENT_META_TYPE_SET_AINSER_GROUP:      return "SetAinSerGroup";
  case MBNG_EVENT_META_TYPE_SET_MF_GROUP:          return "SetMfGroup";
  }

  return "Undefined";
}

mbng_event_meta_type_t MBNG_EVENT_ItemMetaTypeFromStrGet(char *meta_type)
{
  if( strcasecmp(meta_type, "SetButtonGroup") == 0 )     return MBNG_EVENT_META_TYPE_SET_BUTTON_GROUP;
  if( strcasecmp(meta_type, "SetLedGroup") == 0 )        return MBNG_EVENT_META_TYPE_SET_LED_GROUP;
  if( strcasecmp(meta_type, "SetEncGroup") == 0 )        return MBNG_EVENT_META_TYPE_SET_ENC_GROUP;
  if( strcasecmp(meta_type, "SetDinMatrixGroup") == 0 )  return MBNG_EVENT_META_TYPE_SET_DIN_MATRIX_GROUP;
  if( strcasecmp(meta_type, "SetDoutMatrixGroup") == 0 ) return MBNG_EVENT_META_TYPE_SET_DOUT_MATRIX_GROUP;
  if( strcasecmp(meta_type, "SetAinGroup") == 0 )        return MBNG_EVENT_META_TYPE_SET_AIN_GROUP;
  if( strcasecmp(meta_type, "SetAinSerGroup") == 0 )     return MBNG_EVENT_META_TYPE_SET_AINSER_GROUP;
  if( strcasecmp(meta_type, "SetMfGroup") == 0 )         return MBNG_EVENT_META_TYPE_SET_MF_GROUP;
  return MBNG_EVENT_META_TYPE_UNDEFINED;
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
	case MBNG_EVENT_SYSEX_VAR_VAL:        *stream_out = value & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_H:      *stream_out = (value >>  7) & 0x7f; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N1:     *stream_out = (value >>  0) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N2:     *stream_out = (value >>  4) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N3:     *stream_out = (value >>  8) & 0xf; break;
	case MBNG_EVENT_SYSEX_VAR_VAL_N4:     *stream_out = (value >> 12) & 0xf; break;
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
	    MIOS32_MIDI_SendSysEx(port, stream, len);
	  }
	}
      }
    }
    return 0;
  } else if( event_type == MBNG_EVENT_TYPE_RPN ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_NRPN ) {
    // TODO
  } else if( event_type == MBNG_EVENT_TYPE_META ) {
    int i;
    for(i=0; i<item->stream_size; i+=2) {
      mbng_event_meta_type_t meta_type = item->stream[i+0];
      u8 meta_value = item->stream[i+1];
      switch( meta_type ) {
      case MBNG_EVENT_META_TYPE_SET_BUTTON_GROUP:      break;
      case MBNG_EVENT_META_TYPE_SET_LED_GROUP:         break;
      case MBNG_EVENT_META_TYPE_SET_ENC_GROUP:         MBNG_ENC_GroupSet(meta_value-1); break;
      case MBNG_EVENT_META_TYPE_SET_DIN_MATRIX_GROUP:  break;
      case MBNG_EVENT_META_TYPE_SET_DOUT_MATRIX_GROUP: break;
      case MBNG_EVENT_META_TYPE_SET_AIN_GROUP:         break;
      case MBNG_EVENT_META_TYPE_SET_AINSER_GROUP:      break;
      case MBNG_EVENT_META_TYPE_SET_MF_GROUP:          break;
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
s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value)
{
  if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_DEBUG ) {
    MBNG_EVENT_ItemPrint(item);
  }

  switch( item->id & 0xf000 ) {
  //case MBNG_EVENT_CONTROLLER_DISABLED:
  case MBNG_EVENT_CONTROLLER_BUTTON:        return MBNG_DIN_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_LED:           return MBNG_DOUT_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_BUTTON_MATRIX: return MBNG_MATRIX_DIN_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_LED_MATRIX:    return MBNG_MATRIX_DOUT_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_ENC:           return MBNG_ENC_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_AIN:           return MBNG_AIN_NotifyReceivedValue(item, value);
  case MBNG_EVENT_CONTROLLER_AINSER:        return MBNG_AIN_NotifyReceivedValue_SER64(item, value);
  case MBNG_EVENT_CONTROLLER_MF:            return -1; // TODO
  case MBNG_EVENT_CONTROLLER_CV:            return -1; // TODO

  case MBNG_EVENT_CONTROLLER_RECEIVER: {
    int receiver_ix = item->id & 0xfff;

    if( debug_verbose_level >= DEBUG_VERBOSE_LEVEL_INFO ) {
      DEBUG_MSG("MBNG_EVENT_ItemReceive(%d, %d) (Receiver)\n", receiver_ix, value);
      if( item->label )
	DEBUG_MSG(":::%s\n", item->label);
    }

    // print label
    MBNG_LCD_PrintItemLabel(item, value);
  } break;
  }

  return -1; // unsupported controller type
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
      MBNG_EVENT_ItemReceive(&item, value);
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

/////////////////////////////////////////////////////////////////////////////
// This function should be called from APP_SYSEX_Parser on incoming SysEx data
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_EVENT_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in)
{
  // search in pool for matching SysEx streams
  u8 *pool_ptr = (u8 *)&event_pool[0];
  u32 i;
  for(i=0; i<event_pool_num_items; ++i) {
    mbng_event_pool_item_t *pool_item = (mbng_event_pool_item_t *)pool_ptr;
    mbng_event_type_t event_type = ((mbng_event_flags_t)pool_item->flags).general.type;
    if( event_type == MBNG_EVENT_TYPE_SYSEX ) {
      u8 parse_sysex = 1;

      // receiving a SysEx dump=
      if( pool_item->tmp_runtime_flags.sysex_dump ) {
	if( midi_in >= 0xf0 ) {
	  // notify event when all values have been received
	  mbng_event_item_t item;
	  MBNG_EVENT_ItemCopy2User(pool_item, &item);
	  MBNG_EVENT_ItemReceive(&item, pool_item->tmp_sysex_match_ctr); // passing byte counter as value, it could be interesting

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
	      MBNG_EVENT_ItemReceive(&item, pool_item->tmp_sysex_value);
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
