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

// maximum recursion on forwarded events (max 4 can be chained)
#define MBNG_EVENT_MAX_FWD_RECURSION 3


// event pool assignments (upper part of ID)
typedef enum {
  MBNG_EVENT_CONTROLLER_DISABLED      = 0x0000,
  MBNG_EVENT_CONTROLLER_SENDER        = 0x1000,
  MBNG_EVENT_CONTROLLER_RECEIVER      = 0x2000,
  MBNG_EVENT_CONTROLLER_BUTTON        = 0x3000,
  MBNG_EVENT_CONTROLLER_LED           = 0x4000,
  MBNG_EVENT_CONTROLLER_BUTTON_MATRIX = 0x5000,
  MBNG_EVENT_CONTROLLER_LED_MATRIX    = 0x6000,
  MBNG_EVENT_CONTROLLER_ENC           = 0x7000,
  MBNG_EVENT_CONTROLLER_AIN           = 0x8000,
  MBNG_EVENT_CONTROLLER_AINSER        = 0x9000,
  MBNG_EVENT_CONTROLLER_MF            = 0xa000,
  MBNG_EVENT_CONTROLLER_CV            = 0xb000,
} mbng_event_item_id_t;


// event pool assignments (upper part of ID)
typedef enum {
  MBNG_EVENT_TYPE_UNDEFINED = 0,
  MBNG_EVENT_TYPE_NOTE_OFF,
  MBNG_EVENT_TYPE_NOTE_ON,
  MBNG_EVENT_TYPE_POLY_PRESSURE,
  MBNG_EVENT_TYPE_CC,
  MBNG_EVENT_TYPE_PROGRAM_CHANGE,
  MBNG_EVENT_TYPE_AFTERTOUCH,
  MBNG_EVENT_TYPE_PITCHBEND,
  MBNG_EVENT_TYPE_SYSEX,
  MBNG_EVENT_TYPE_NRPN,
  MBNG_EVENT_TYPE_META,
} mbng_event_type_t;

typedef enum {
  MBNG_EVENT_BUTTON_MODE_UNDEFINED = 0,
  MBNG_EVENT_BUTTON_MODE_ON_OFF,
  MBNG_EVENT_BUTTON_MODE_ON_ONLY,
  MBNG_EVENT_BUTTON_MODE_TOGGLE,
} mbng_event_button_mode_t;

typedef enum {
  MBNG_EVENT_ENC_MODE_UNDEFINED = 0,
  MBNG_EVENT_ENC_MODE_ABSOLUTE,
  MBNG_EVENT_ENC_MODE_40SPEED,
  MBNG_EVENT_ENC_MODE_00SPEED,
  MBNG_EVENT_ENC_MODE_INC00SPEED_DEC40SPEED,
  MBNG_EVENT_ENC_MODE_INC41_DEC3F,
  MBNG_EVENT_ENC_MODE_INC01_DEC7F,
  MBNG_EVENT_ENC_MODE_INC01_DEC41,
  MBNG_EVENT_ENC_MODE_INC_DEC,
} mbng_event_enc_mode_t;

typedef enum {
  MBNG_EVENT_ENC_SPEED_MODE_UNDEFINED = 0,
  MBNG_EVENT_ENC_SPEED_MODE_AUTO,
  MBNG_EVENT_ENC_SPEED_MODE_SLOW,
  MBNG_EVENT_ENC_SPEED_MODE_NORMAL,
  MBNG_EVENT_ENC_SPEED_MODE_FAST,
} mbng_event_enc_speed_mode_t;

typedef enum {
  MBNG_EVENT_LED_MATRIX_PATTERN_UNDEFINED = 0,
  MBNG_EVENT_LED_MATRIX_PATTERN_1,
  MBNG_EVENT_LED_MATRIX_PATTERN_2,
  MBNG_EVENT_LED_MATRIX_PATTERN_3,
  MBNG_EVENT_LED_MATRIX_PATTERN_4,
  MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO,
} mbng_event_led_matrix_pattern_t;

typedef enum {
  MBNG_EVENT_AIN_MODE_UNDEFINED = 0,
  MBNG_EVENT_AIN_MODE_DIRECT,
  MBNG_EVENT_AIN_MODE_SNAP,
  MBNG_EVENT_AIN_MODE_RELATIVE,
  MBNG_EVENT_AIN_MODE_PARALLAX,
} mbng_event_ain_mode_t;

typedef enum {
  MBNG_EVENT_NRPN_FORMAT_UNDEFINED = 0,
  MBNG_EVENT_NRPN_FORMAT_UNSIGNED,
  MBNG_EVENT_NRPN_FORMAT_SIGNED,
} mbng_event_nrpn_format_t;

typedef enum {
  MBNG_EVENT_SYSEX_VAR_UNDEFINED = 0,
  MBNG_EVENT_SYSEX_VAR_DEV,
  MBNG_EVENT_SYSEX_VAR_PAT,
  MBNG_EVENT_SYSEX_VAR_BNK,
  MBNG_EVENT_SYSEX_VAR_INS,
  MBNG_EVENT_SYSEX_VAR_CHN,
  MBNG_EVENT_SYSEX_VAR_CHK_START,
  MBNG_EVENT_SYSEX_VAR_CHK,
  MBNG_EVENT_SYSEX_VAR_CHK_INV,
  MBNG_EVENT_SYSEX_VAR_VAL,
  MBNG_EVENT_SYSEX_VAR_VAL_H,
  MBNG_EVENT_SYSEX_VAR_VAL_N1,
  MBNG_EVENT_SYSEX_VAR_VAL_N2,
  MBNG_EVENT_SYSEX_VAR_VAL_N3,
  MBNG_EVENT_SYSEX_VAR_VAL_N4,
  MBNG_EVENT_SYSEX_VAR_IGNORE,
  MBNG_EVENT_SYSEX_VAR_DUMP,
  MBNG_EVENT_SYSEX_VAR_CURSOR,
  MBNG_EVENT_SYSEX_VAR_TXT,
  MBNG_EVENT_SYSEX_VAR_TXT56,
} mbng_event_sysex_var_t;

typedef enum {
  MBNG_EVENT_META_TYPE_UNDEFINED = 0,

  MBNG_EVENT_META_TYPE_SET_BANK,
  MBNG_EVENT_META_TYPE_DEC_BANK,
  MBNG_EVENT_META_TYPE_INC_BANK,
  MBNG_EVENT_META_TYPE_CYCLE_BANK,

  MBNG_EVENT_META_TYPE_SET_BANK_OF_HW_ID,
  MBNG_EVENT_META_TYPE_DEC_BANK_OF_HW_ID,
  MBNG_EVENT_META_TYPE_INC_BANK_OF_HW_ID,
  MBNG_EVENT_META_TYPE_CYCLE_BANK_OF_HW_ID,

  MBNG_EVENT_META_TYPE_ENC_FAST,

  MBNG_EVENT_META_TYPE_MIDI_LEARN,

  MBNG_EVENT_META_TYPE_SCS_ENC,
  MBNG_EVENT_META_TYPE_SCS_MENU,
  MBNG_EVENT_META_TYPE_SCS_SOFT1,
  MBNG_EVENT_META_TYPE_SCS_SOFT2,
  MBNG_EVENT_META_TYPE_SCS_SOFT3,
  MBNG_EVENT_META_TYPE_SCS_SOFT4,
  MBNG_EVENT_META_TYPE_SCS_SHIFT,
} mbng_event_meta_type_t;


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u32 ALL;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
  } general;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 radio_group:6;
  } SENDER;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 radio_group:6;
  } RECEIVER;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 radio_group:6;
    u32 button_mode:2; // mbng_event_button_mode_t
  } DIN;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 radio_group:6;
  } DOUT;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 mapped:1;
  } BUTTON_MATRIX;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
  } LED_MATRIX;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 enc_mode:4; // mbng_event_enc_mode_t
    u32 enc_speed_mode:3; // mbng_event_enc_speed_mode_t
    u32 enc_speed_mode_par:3;
  } ENC;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 ain_mode:4; // mbng_event_ain_mode_t
  } AIN;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 ain_mode:4; // mbng_event_ain_mode_t
  } AINSER;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:3; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 active:1;
    u32 fwd_gate_to_dout_pin:9;
    u32 cv_inverted:1;
    u32 cv_hz_v:1;
    u32 cv_gate_inverted:1;
  } CV;

} mbng_event_flags_t;


typedef union {
  u16 ALL;

  struct {
    u16 pos:12;
    u16 receiver:4;
  };
} mbng_event_syxdump_pos_t;


typedef struct {
  u16 id;
  u16 hw_id;
  u16 pool_address;
  u16 enabled_ports;
  mbng_event_flags_t flags;
  u16 fwd_id;
  u16 value;
  s16 min;
  s16 max;
  s16 offset;
  u16 matrix_pin;
  mbng_event_syxdump_pos_t syxdump_pos;
  u32 stream_size;
  u8* stream;
  u8 map;
  u8 bank;
  u8 secondary_value;
  u8 lcd;
  u8 lcd_pos;
  char *label;
} mbng_event_item_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_EVENT_Init(u32 mode);

extern s32 MBNG_EVENT_Tick(void);

extern s32 MBNG_EVENT_PoolClear(void);
extern s32 MBNG_EVENT_PoolUpdate(void);
extern s32 MBNG_EVENT_PoolPrint(void);
extern s32 MBNG_EVENT_PoolItemsPrint(void);
extern s32 MBNG_EVENT_PoolMapsPrint(void);

extern s32 MBNG_EVENT_PoolNumItemsGet(void);
extern s32 MBNG_EVENT_PoolNumMapsGet(void);
extern s32 MBNG_EVENT_PoolSizeGet(void);
extern s32 MBNG_EVENT_PoolMaxSizeGet(void);

extern s32 MBNG_EVENT_MapAdd(u8 map, u8 *map_values, u8 len);
extern s32 MBNG_EVENT_MapGet(u8 map, u8 **map_values);
extern s32 MBNG_EVENT_MapIxGet(u8 *map_values, u8 map_len, u8 value);

extern s32 MBNG_EVENT_NumBanksGet(void);
extern s32 MBNG_EVENT_SelectedBankGet(void);
extern s32 MBNG_EVENT_SelectedBankSet(u8 new_bank);
extern s32 MBNG_EVENT_HwIdBankGet(u16 hw_id);
extern s32 MBNG_EVENT_HwIdBankSet(u16 hw_id, u8 new_bank);

extern s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item, mbng_event_item_id_t id);
extern s32 MBNG_EVENT_ItemGet(u32 item_ix, mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemAdd(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemModify(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSearchById(mbng_event_item_id_t id, mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSearchByHwId(mbng_event_item_id_t hw_id, mbng_event_item_t *item);

extern s32 MBNG_EVENT_MidiLearnModeSet(u8 mode);
extern s32 MBNG_EVENT_MidiLearnModeGet(void);
extern s32 MBNG_EVENT_MidiLearnStatusMsg(char *line1, char *line2);
extern s32 MBNG_EVENT_MidiLearnIt(mbng_event_item_id_t hw_id);


extern s32 MBNG_EVENT_ItemPrint(mbng_event_item_t *item);
extern const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_id_t id);
extern mbng_event_item_id_t MBNG_EVENT_ItemIdFromControllerStrGet(char *event);
extern const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item);
extern mbng_event_type_t MBNG_EVENT_ItemTypeFromStrGet(char *event_type);
extern const char *MBNG_EVENT_ItemButtonModeStrGet(mbng_event_item_t *item);
extern mbng_event_button_mode_t MBNG_EVENT_ItemButtonModeFromStrGet(char *button_mode);
extern const char *MBNG_EVENT_ItemAinModeStrGet(mbng_event_item_t *item);
extern mbng_event_ain_mode_t MBNG_EVENT_ItemAinModeFromStrGet(char *ain_mode);
extern const char *MBNG_EVENT_ItemEncModeStrGet(mbng_event_item_t *item);
extern mbng_event_enc_mode_t MBNG_EVENT_ItemEncModeFromStrGet(char *enc_mode);
extern const char *MBNG_EVENT_ItemEncSpeedModeStrGet(mbng_event_item_t *item);
extern mbng_event_enc_speed_mode_t MBNG_EVENT_ItemEncSpeedModeFromStrGet(char *enc_speed_mode);
extern const char *MBNG_EVENT_ItemLedMatrixPatternStrGet(mbng_event_item_t *item);
extern mbng_event_led_matrix_pattern_t MBNG_EVENT_ItemLedMatrixPatternFromStrGet(char *led_matrix_pattern);
extern const char *MBNG_EVENT_ItemNrpnFormatStrGet(mbng_event_item_t *item);
extern mbng_event_nrpn_format_t MBNG_EVENT_ItemNrpnFormatFromStrGet(char *nrpn_format);
extern const char *MBNG_EVENT_ItemSysExVarStrGet(mbng_event_item_t *item, u8 stream_pos);
extern mbng_event_sysex_var_t MBNG_EVENT_ItemSysExVarFromStrGet(char *sysex_var);
extern const char *MBNG_EVENT_ItemMetaTypeStrGet(mbng_event_item_t *item, u8 entry);
extern mbng_event_meta_type_t MBNG_EVENT_ItemMetaTypeFromStrGet(char *meta_type);


extern s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value, u8 from_midi);
extern s32 MBNG_EVENT_ItemForward(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemForwardToRadioGroup(mbng_event_item_t *item, u8 radio_group);

extern s32 MBNG_EVENT_NotifySendValue(mbng_event_item_t *item);
extern s32 MBNG_EVENT_Refresh(void);

extern s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 MBNG_EVENT_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_EVENT_H */
