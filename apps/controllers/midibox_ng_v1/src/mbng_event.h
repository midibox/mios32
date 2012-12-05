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

#ifndef _MBNG_EVENT_H
#define _MBNG_EVENT_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// event pool assignments (upper part of ID)
typedef enum {
  MBNG_EVENT_CONTROLLER_DISABLED      = 0x0000,
  MBNG_EVENT_CONTROLLER_BUTTON        = 0x1000,
  MBNG_EVENT_CONTROLLER_LED           = 0x2000,
  MBNG_EVENT_CONTROLLER_BUTTON_MATRIX = 0x3000,
  MBNG_EVENT_CONTROLLER_LED_MATRIX    = 0x4000,
  MBNG_EVENT_CONTROLLER_ENC           = 0x5000,
  MBNG_EVENT_CONTROLLER_AIN           = 0x6000,
  MBNG_EVENT_CONTROLLER_AINSER        = 0x7000,
  MBNG_EVENT_CONTROLLER_MF            = 0x8000,
  MBNG_EVENT_CONTROLLER_CV            = 0x9000,
} mbng_event_item_id_t;


// event pool assignments (upper part of ID)
typedef enum {
  MBNG_EVENT_TYPE_NOTE_OFF       = 0,
  MBNG_EVENT_TYPE_NOTE_ON        = 1,
  MBNG_EVENT_TYPE_POLY_PRESSURE  = 2,
  MBNG_EVENT_TYPE_CC             = 3,
  MBNG_EVENT_TYPE_PROGRAM_CHANGE = 4,
  MBNG_EVENT_TYPE_AFTERTOUCH     = 5,
  MBNG_EVENT_TYPE_PITCHBEND      = 6,
  MBNG_EVENT_TYPE_SYSEX          = 7,
  MBNG_EVENT_TYPE_RPN            = 8,
  MBNG_EVENT_TYPE_NRPN           = 9,
  MBNG_EVENT_TYPE_META           = 10,
} mbng_event_type_t;

typedef enum {
  MBNG_EVENT_BUTTON_MODE_ON_OFF  = 0,
  MBNG_EVENT_BUTTON_MODE_ON_ONLY = 1,
  MBNG_EVENT_BUTTON_MODE_TOGGLE  = 2,
} mbng_event_button_mode_t;

typedef enum {
  MBNG_EVENT_ENC_MODE_ABSOLUTE = 0,
  MBNG_EVENT_ENC_MODE_40SPEED  = 1,
  MBNG_EVENT_ENC_MODE_00SPEED  = 2,
  MBNG_EVENT_ENC_MODE_40_1     = 3,
  MBNG_EVENT_ENC_MODE_00_1     = 4,
  MBNG_EVENT_ENC_MODE_INC_DEC  = 5,
} mbng_event_enc_mode_t;

typedef enum {
  MBNG_EVENT_AIN_MODE_DIRECT   = 0,
  MBNG_EVENT_AIN_MODE_SNAP     = 1,
  MBNG_EVENT_AIN_MODE_RELATIVE = 2,
  MBNG_EVENT_AIN_MODE_PARALLAX = 3,
} mbng_event_ain_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u16 ALL;

  struct {
    u16 type:4;
  } general;

  struct {
    u16 type:4;
    u16 inverse:1;
    u16 button_mode:2; // mbng_event_button_mode_t
    u16 radio_group:8;
  } DIN;

  struct {
    u16 type:4;
    u16 inverse:1;
    u16 radio_group:8;
  } DOUT;

  struct {
    u16 type:4;
    u16 inverse:1;
    u16 mapped:1;
  } DIN_MATRIX;

  struct {
    u16 type:4;
    u16 inverse:1;
  } DOUT_MATRIX;

  struct {
    u16 type:4;
    u16 enc_mode:4; // mbng_event_enc_mode_t
  } ENC;

  struct {
    u16 type:4;
    u16 ain_mode:4; // mbng_event_ain_mode_t
  } AIN;

  struct {
    u16 type:4;
    u16 ain_mode:4; // mbng_event_ain_mode_t
  } AINSER;
} mbng_event_flags_t;


typedef struct {
  u16 id;
  mbng_event_flags_t flags;
  u16 enabled_ports;
  u16 min;
  u16 max;
  u32 stream_size;
  u8* stream;
  u8 lcd_num;
  u8 lcd_pos;
  char *label;
} mbng_event_item_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_EVENT_Init(u32 mode);

extern s32 MBNG_EVENT_PoolClear(void);

extern s32 MBNG_EVENT_PoolPrint(void);

extern s32 MBNG_EVENT_PoolNumItemsGet(void);
extern s32 MBNG_EVENT_PoolSizeGet(void);
extern s32 MBNG_EVENT_PoolMaxSizeGet(void);

extern s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemAdd(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSearchById(mbng_event_item_id_t id, mbng_event_item_t *item);

extern s32 MBNG_EVENT_ItemPrint(mbng_event_item_t *item);
extern const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_t *item);
extern const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item);

extern s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item, u16 value);
extern s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value);

extern s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_EVENT_H */
