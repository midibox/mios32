// $Id: mbng_event.h 2019 2014-06-04 19:09:46Z tk $
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
  MBNG_EVENT_CONTROLLER_KB            = 0xc000,
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
  MBNG_EVENT_TYPE_MBFM,
} mbng_event_type_t;

typedef enum {
  MBNG_EVENT_IF_COND_NONE = 0,
  MBNG_EVENT_IF_COND_EQ,
  MBNG_EVENT_IF_COND_EQ_STOP_ON_MATCH,
  MBNG_EVENT_IF_COND_UNEQ,
  MBNG_EVENT_IF_COND_UNEQ_STOP_ON_MATCH,
  MBNG_EVENT_IF_COND_LT,
  MBNG_EVENT_IF_COND_LT_STOP_ON_MATCH,
  MBNG_EVENT_IF_COND_LEQ,
  MBNG_EVENT_IF_COND_LEQ_STOP_ON_MATCH,
} mbng_event_if_cond_t;

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
  MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT1,
  MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT2,
  MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT3,
  MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT4,
  MBNG_EVENT_LED_MATRIX_PATTERN_DIGIT5,
  MBNG_EVENT_LED_MATRIX_PATTERN_LC_DIGIT,
  MBNG_EVENT_LED_MATRIX_PATTERN_LC_AUTO,
} mbng_event_led_matrix_pattern_t;

typedef enum {
  MBNG_EVENT_AIN_MODE_UNDEFINED = 0,
  MBNG_EVENT_AIN_MODE_DIRECT,
  MBNG_EVENT_AIN_MODE_SNAP,
  MBNG_EVENT_AIN_MODE_RELATIVE,
  MBNG_EVENT_AIN_MODE_PARALLAX,
  MBNG_EVENT_AIN_MODE_SWITCH,
} mbng_event_ain_mode_t;

typedef enum {
  MBNG_EVENT_AIN_SENSOR_MODE_NONE = 0,
  MBNG_EVENT_AIN_SENSOR_MODE_NOTE_ON_OFF,
} mbng_event_ain_sensor_mode_t;

typedef enum {
  MBNG_EVENT_NRPN_FORMAT_UNDEFINED = 0,
  MBNG_EVENT_NRPN_FORMAT_UNSIGNED,
  MBNG_EVENT_NRPN_FORMAT_SIGNED,
  MBNG_EVENT_NRPN_FORMAT_MSB_ONLY,
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
  MBNG_EVENT_SYSEX_VAR_LABEL,
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

  MBNG_EVENT_META_TYPE_SET_SNAPSHOT,
  MBNG_EVENT_META_TYPE_DEC_SNAPSHOT,
  MBNG_EVENT_META_TYPE_INC_SNAPSHOT,
  MBNG_EVENT_META_TYPE_CYCLE_SNAPSHOT,
  MBNG_EVENT_META_TYPE_LOAD_SNAPSHOT,
  MBNG_EVENT_META_TYPE_SAVE_SNAPSHOT,
  MBNG_EVENT_META_TYPE_DUMP_SNAPSHOT,

  MBNG_EVENT_META_TYPE_RETRIEVE_AIN_VALUES,
  MBNG_EVENT_META_TYPE_RETRIEVE_AINSER_VALUES,

  MBNG_EVENT_META_TYPE_ENC_FAST,

  MBNG_EVENT_META_TYPE_MIDI_LEARN,

  MBNG_EVENT_META_TYPE_UPDATE_LCD,

  MBNG_EVENT_META_TYPE_RESET_METERS,

  MBNG_EVENT_META_TYPE_SWAP_VALUES,

  MBNG_EVENT_META_TYPE_RUN_SECTION,
  MBNG_EVENT_META_TYPE_RUN_STOP,

  MBNG_EVENT_META_TYPE_MCLK_PLAY,
  MBNG_EVENT_META_TYPE_MCLK_STOP,
  MBNG_EVENT_META_TYPE_MCLK_PLAYSTOP,
  MBNG_EVENT_META_TYPE_MCLK_PAUSE,
  MBNG_EVENT_META_TYPE_MCLK_SET_TEMPO,
  MBNG_EVENT_META_TYPE_MCLK_DEC_TEMPO,
  MBNG_EVENT_META_TYPE_MCLK_INC_TEMPO,

  MBNG_EVENT_META_TYPE_CV_PITCHBEND_14BIT,
  MBNG_EVENT_META_TYPE_CV_PITCHBEND_7BIT,
  MBNG_EVENT_META_TYPE_CV_PITCHRANGE,
  MBNG_EVENT_META_TYPE_CV_TRANSPOSE_OCTAVE,
  MBNG_EVENT_META_TYPE_CV_TRANSPOSE_SEMITONES,

  /*
  MBNG_EVENT_META_TYPE_SCS_ENC,
  MBNG_EVENT_META_TYPE_SCS_MENU,
  MBNG_EVENT_META_TYPE_SCS_SOFT1,
  MBNG_EVENT_META_TYPE_SCS_SOFT2,
  MBNG_EVENT_META_TYPE_SCS_SOFT3,
  MBNG_EVENT_META_TYPE_SCS_SOFT4,
  MBNG_EVENT_META_TYPE_SCS_SOFT5,
  MBNG_EVENT_META_TYPE_SCS_SOFT6,
  MBNG_EVENT_META_TYPE_SCS_SOFT7,
  MBNG_EVENT_META_TYPE_SCS_SOFT8,
  MBNG_EVENT_META_TYPE_SCS_SHIFT,
  */
} mbng_event_meta_type_t;


typedef enum {
  MBNG_EVENT_MBFM_TYPE_UNDEFINED = 0,
  
  MBNG_EVENT_MBFM_TYPE_SELECTOPL3, //[RANGE=0:1]
  MBNG_EVENT_MBFM_TYPE_SELECTVOICE, //[INDEX=0:17]
  MBNG_EVENT_MBFM_TYPE_SELECTMODE, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNSOFTKEY, //[INDEX=1:8]
  MBNG_EVENT_MBFM_TYPE_BTNSELECT,
  MBNG_EVENT_MBFM_TYPE_BTNMENU,
  MBNG_EVENT_MBFM_TYPE_BTNDUPL,
  MBNG_EVENT_MBFM_TYPE_BTNLINK,
  MBNG_EVENT_MBFM_TYPE_BTNRETRIG,
  MBNG_EVENT_MBFM_TYPE_BTNDLYSCALE,
  MBNG_EVENT_MBFM_TYPE_BTN4OP,
  MBNG_EVENT_MBFM_TYPE_BTNALG,
  MBNG_EVENT_MBFM_TYPE_BTNDEST, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_BTNOPPARAM, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNOPWAVE, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNOPMUTE, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNEGASSIGN, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_BTNLFOASSIGN, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNLFOWAVE, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_BTNWTASSIGN, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_BTNVELASSIGN,
  MBNG_EVENT_MBFM_TYPE_BTNMODASSIGN,
  MBNG_EVENT_MBFM_TYPE_BTNVARIASSIGN,
  MBNG_EVENT_MBFM_TYPE_BTNSEQREC,
  MBNG_EVENT_MBFM_TYPE_BTNSEQPLAY,
  MBNG_EVENT_MBFM_TYPE_BTNSEQBACK,
  MBNG_EVENT_MBFM_TYPE_BTNSEQMUTE,
  MBNG_EVENT_MBFM_TYPE_BTNSEQSOLO,
  MBNG_EVENT_MBFM_TYPE_BTNSEQVEL,
  MBNG_EVENT_MBFM_TYPE_BTNSEQVARI,
  MBNG_EVENT_MBFM_TYPE_BTNSEQGATE,
  MBNG_EVENT_MBFM_TYPE_BTNSEQTRIGGER, //[INDEX=0:11]
  MBNG_EVENT_MBFM_TYPE_BTNSEQMEASURE, //[INDEX=1:8]
  MBNG_EVENT_MBFM_TYPE_BTNSEQSECTION,

  MBNG_EVENT_MBFM_TYPE_SETDATAWHEEL,
  MBNG_EVENT_MBFM_TYPE_SETVOICETRANSPOSE,
  MBNG_EVENT_MBFM_TYPE_SETVOICETUNE,
  MBNG_EVENT_MBFM_TYPE_SETVOICEPORTA,
  MBNG_EVENT_MBFM_TYPE_SETVOICEDLYTIME,
  MBNG_EVENT_MBFM_TYPE_SETVOICEFB,
  MBNG_EVENT_MBFM_TYPE_SETOPFMULT, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETOPATK, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETOPDEC, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETOPSUS, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETOPREL, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETOPVOLUME, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETEGATK, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETEGDEC1, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETEGLEVEL, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETEGDEC2, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETEGSUS, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETEGREL, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_SETLFOFREQ, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SETLFODELAY, //[INDEX=0:3]
  
  MBNG_EVENT_MBFM_TYPE_INCDATAWHEEL,
  MBNG_EVENT_MBFM_TYPE_INCVOICETRANSPOSE,
  MBNG_EVENT_MBFM_TYPE_INCVOICETUNE,
  MBNG_EVENT_MBFM_TYPE_INCVOICEPORTA,
  MBNG_EVENT_MBFM_TYPE_INCVOICEDLYTIME,
  MBNG_EVENT_MBFM_TYPE_INCVOICEFB,
  MBNG_EVENT_MBFM_TYPE_INCOPFMULT, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCOPATK, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCOPDEC, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCOPSUS, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCOPREL, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCOPVOLUME, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCEGATK, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCEGDEC1, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCEGLEVEL, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCEGDEC2, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCEGSUS, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCEGREL, //[INDEX=0:1]
  MBNG_EVENT_MBFM_TYPE_INCLFOFREQ, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_INCLFODELAY, //[INDEX=0:3]

  MBNG_EVENT_MBFM_TYPE_DISPVOICE,
  MBNG_EVENT_MBFM_TYPE_DISPVALUE,
  MBNG_EVENT_MBFM_TYPE_CLEARVOICE,
  MBNG_EVENT_MBFM_TYPE_CLEARVALUE,
  MBNG_EVENT_MBFM_TYPE_VOICELEDRED, //[INDEX=0:17]
  MBNG_EVENT_MBFM_TYPE_VOICELEDGREEN, //[INDEX=0:17]
  MBNG_EVENT_MBFM_TYPE_ALGOPLEDRED, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_ALGOPLEDGREEN, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_SEQTRIGGERLED, //[INDEX=0:11]
  MBNG_EVENT_MBFM_TYPE_ISMODE, //[INDEX=0:3]
  MBNG_EVENT_MBFM_TYPE_ISDUPL,
  MBNG_EVENT_MBFM_TYPE_ISLINK,
  MBNG_EVENT_MBFM_TYPE_ISRETRIG,
  MBNG_EVENT_MBFM_TYPE_ISDLYSCALE,
  MBNG_EVENT_MBFM_TYPE_ISCURVOICE4OP,
  MBNG_EVENT_MBFM_TYPE_ISSOMEVOICE4OP, //[INDEX=0:5]
  MBNG_EVENT_MBFM_TYPE_ISRECORDING,
  MBNG_EVENT_MBFM_TYPE_ISPLAYING,
  MBNG_EVENT_MBFM_TYPE_ISMUTE,
  MBNG_EVENT_MBFM_TYPE_ISSOLO,
  MBNG_EVENT_MBFM_TYPE_ISMEASURE, //[INDEX=1:8]
  MBNG_EVENT_MBFM_TYPE_ISSEQVEL,
  MBNG_EVENT_MBFM_TYPE_ISSEQVARI,
  MBNG_EVENT_MBFM_TYPE_ISSEQGATE,
  MBNG_EVENT_MBFM_TYPE_ISALGFB,
  MBNG_EVENT_MBFM_TYPE_ISALGFM, //[INDEX=0:2]
  MBNG_EVENT_MBFM_TYPE_ISALGOUT, //[INDEX=0:3]

  MBNG_EVENT_MBFM_TYPE_CONTROLMOD, //[INDEX=1:127?]
  MBNG_EVENT_MBFM_TYPE_CONTROLVARI, //[INDEX=1:127?]
  MBNG_EVENT_MBFM_TYPE_CONTROLSUS, //[INDEX=1:127]
} mbng_event_mbfm_type_t;



/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u32 ALL;

  struct {
    u32 type:4;
    u32 led_matrix_pattern:4; // mbng_event_led_matrix_pattern_t
    u32 fwd_to_lcd:1;
    u32 update_lcd:1;
    u32 value_from_midi:1;
    u32 use_key_or_cc:1;
    u32 use_any_key_or_cc:1;
    u32 active:1;
    u32 write_locked:1;
    u32 no_dump:1;
    u32 dimmed:1;
    u32 radio_group:6;
    u32 colour:2;
    u32 digit_blankzero:1;
    u32 digit_signed:1;
  };

} mbng_event_flags_t;


typedef union {
  u16 ALL;

  //struct {
  //} SENDER;

  struct {
    u16 emu_enc_mode:4; // mbng_event_enc_mode_t
    u16 emu_enc_hw_id:8;
  } RECEIVER;

  struct {
    u16 button_mode:2; // mbng_event_button_mode_t
  } DIN;

  //struct {
  //} DOUT;

  struct {
    u16 mapped:1;
  } BUTTON_MATRIX;

  //struct {
  //} LED_MATRIX;

  struct {
    u16 enc_mode:4; // mbng_event_enc_mode_t
    u16 enc_speed_mode:3; // mbng_event_enc_speed_mode_t
    u16 enc_speed_mode_par:3;
  } ENC;

  struct {
    u16 ain_mode:4; // mbng_event_ain_mode_t
    u16 ain_sensor_mode:4;  // mbng_event_ain_sensor_mode_t
    u16 ain_filter_delay_ms:8;
  } AIN;

  struct {
    u16 ain_mode:4; // mbng_event_ain_mode_t
    u16 ain_sensor_mode:4;  // mbng_event_ain_sensor_mode_t
  } AINSER;

  struct {
    u16 fwd_gate_to_dout_pin:9;
    u16 cv_inverted:1;
    u16 cv_hz_v:1;
    u16 cv_gate_inverted:1;
  } CV;

  struct {
    u16 kb_transpose:8; // has to be converted to a signed value
    u16 kb_velocity_map:8;
  } KB;

} mbng_event_custom_flags_t;


typedef union {
  u32 ALL;

  struct {
    u32 condition:4;
    u32 hw_id:14;
    u32 value:14;
  };
} mbng_event_cond_t;


typedef union {
  u32 ALL;

  struct {
    u16 pos:12;
    u16 receiver:12;
  };
} mbng_event_syxdump_pos_t;

typedef union {
  u16 ALL;

  struct {
    u16 r:4;
    u16 g:4;
    u16 b:4;
  };
} mbng_event_rgb_t;

typedef struct {
  u16 id;
  u16 hw_id;
  mbng_event_cond_t cond;
  u16 pool_address;
  u16 enabled_ports;
  mbng_event_flags_t flags;
  mbng_event_custom_flags_t custom_flags;
  u16 fwd_id;
  u16 value;
  u16 fwd_value;
  s16 min;
  s16 max;
  s16 offset;
  u16 matrix_pin;
  mbng_event_rgb_t rgb;
  mbng_event_syxdump_pos_t syxdump_pos;
  u32 stream_size;
  u8* stream;
  u8 map;
  u8 bank;
  u8 secondary_value;
  u8 lcd;
  u8 lcd_x;
  u8 lcd_y;
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

extern s32 MBNG_EVENT_ItemNoDumpDefault(mbng_event_item_t *item);

extern s32 MBNG_EVENT_ItemInit(mbng_event_item_t *item, mbng_event_item_id_t id);
extern s32 MBNG_EVENT_ItemGet(u32 item_ix, mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemAdd(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemModify(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSearchById(mbng_event_item_id_t id, mbng_event_item_t *item, u32 *continue_ix);
extern s32 MBNG_EVENT_ItemSearchByHwId(mbng_event_item_id_t hw_id, mbng_event_item_t *item, u32 *continue_ix);
extern s32 MBNG_EVENT_ItemRetrieveValues(mbng_event_item_id_t *id, s16 *value, u8 *secondary_value, u32 *continue_ix);
extern s32 MBNG_EVENT_ItemCopyValueToPool(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSetLock(mbng_event_item_t *item, u8 lock);
extern s32 MBNG_EVENT_ItemSetActive(mbng_event_item_t *item, u8 active);
extern s32 MBNG_EVENT_ItemCheckMatchingCondition(mbng_event_item_t *item);

extern s32 MBNG_EVENT_LCMeters_Init(void);

extern s32 MBNG_EVENT_MidiLearnModeSet(u8 mode);
extern s32 MBNG_EVENT_MidiLearnModeGet(void);
extern s32 MBNG_EVENT_MidiLearnStatusMsg(char *line1, char *line2);
extern s32 MBNG_EVENT_MidiLearnIt(mbng_event_item_id_t hw_id);


extern s32 MBNG_EVENT_ItemPrint(mbng_event_item_t *item, u8 all);
extern s32 MBNG_EVENT_ItemSearchByIdAndPrint(mbng_event_item_id_t id);
extern s32 MBNG_EVENT_ItemSearchByHwIdAndPrint(mbng_event_item_id_t hw_id);

extern const char *MBNG_EVENT_ItemControllerStrGet(mbng_event_item_id_t id);
extern mbng_event_item_id_t MBNG_EVENT_ItemIdFromControllerStrGet(char *event);
extern const char *MBNG_EVENT_ItemTypeStrGet(mbng_event_item_t *item);
extern mbng_event_type_t MBNG_EVENT_ItemTypeFromStrGet(char *event_type);
extern const char *MBNG_EVENT_ItemConditionStrGet(mbng_event_item_t *item);
extern mbng_event_if_cond_t MBNG_EVENT_ItemConditionFromStrGet(char *condition);
extern const char *MBNG_EVENT_ItemButtonModeStrGet(mbng_event_item_t *item);
extern mbng_event_button_mode_t MBNG_EVENT_ItemButtonModeFromStrGet(char *button_mode);
extern const char *MBNG_EVENT_ItemAinModeStrGet(mbng_event_item_t *item);
extern mbng_event_ain_mode_t MBNG_EVENT_ItemAinModeFromStrGet(char *ain_mode);
extern const char *MBNG_EVENT_ItemAinSensorModeStrGet(mbng_event_item_t *item);
extern mbng_event_ain_sensor_mode_t MBNG_EVENT_ItemAinSensorModeFromStrGet(char *ain_sensor_mode);
extern const char *MBNG_EVENT_ItemEncModeStrGet(mbng_event_enc_mode_t enc_mode);
extern mbng_event_enc_mode_t MBNG_EVENT_ItemEncModeFromStrGet(char *enc_mode);
extern const char *MBNG_EVENT_ItemEncSpeedModeStrGet(mbng_event_item_t *item);
extern mbng_event_enc_speed_mode_t MBNG_EVENT_ItemEncSpeedModeFromStrGet(char *enc_speed_mode);
extern const char *MBNG_EVENT_ItemLedMatrixPatternStrGet(mbng_event_item_t *item);
extern mbng_event_led_matrix_pattern_t MBNG_EVENT_ItemLedMatrixPatternFromStrGet(char *led_matrix_pattern);
extern const char *MBNG_EVENT_ItemNrpnFormatStrGet(mbng_event_item_t *item);
extern mbng_event_nrpn_format_t MBNG_EVENT_ItemNrpnFormatFromStrGet(char *nrpn_format);
extern const char *MBNG_EVENT_ItemSysExVarStrGet(mbng_event_item_t *item, u8 stream_pos);
extern mbng_event_sysex_var_t MBNG_EVENT_ItemSysExVarFromStrGet(char *sysex_var);
extern const char *MBNG_EVENT_ItemMetaTypeStrGet(mbng_event_meta_type_t meta_type);
extern mbng_event_meta_type_t MBNG_EVENT_ItemMetaTypeFromStrGet(char *meta_type);
extern u8 MBNG_EVENT_ItemMetaNumBytesGet(mbng_event_meta_type_t meta_type);
extern const char *MBNG_EVENT_ItemMBFMTypeStrGet(mbng_event_mbfm_type_t mbfm_type);
extern mbng_event_mbfm_type_t MBNG_EVENT_ItemMBFMTypeFromStrGet(char *mbfm_type);
extern u8 MBNG_EVENT_ItemMBFMNumBytesGet(mbng_event_mbfm_type_t mbfm_type);


extern s32 MBNG_EVENT_SendOptimizedNRPN(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 nrpn_address, u16 nrpn_value, u8 msb_only);
extern s32 MBNG_EVENT_SendSysExStream(mios32_midi_port_t port, mbng_event_item_t *item);
extern s32 MBNG_EVENT_ExecMeta(mbng_event_item_t *item);

extern s32 MBNG_EVENT_ExecMBFM(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ReceiveMBFM(mbng_event_mbfm_type_t mbfm_type, u8 index, u16 value);

extern s32 MBNG_EVENT_ItemSend(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemSendVirtual(mbng_event_item_t *item, mbng_event_item_id_t send_id);

extern s32 MBNG_EVENT_ItemReceive(mbng_event_item_t *item, u16 value, u8 from_midi, u8 fwd_enabled);
extern s32 MBNG_EVENT_ItemForward(mbng_event_item_t *item);
extern s32 MBNG_EVENT_ItemForwardToRadioGroup(mbng_event_item_t *item, u8 radio_group);

extern s32 MBNG_EVENT_NotifySendValue(mbng_event_item_t *item);

extern s32 MBNG_EVENT_Refresh(void);
extern s32 MBNG_EVENT_UpdateLCD(u8 force);

extern s32 MBNG_EVENT_Dump(void);

extern s32 MBNG_EVENT_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 MBNG_EVENT_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in);



/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_EVENT_H */
