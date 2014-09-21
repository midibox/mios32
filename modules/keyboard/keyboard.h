// $Id$
/*
 * Header file for Keyboard handler
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of keyboards handled in parallel
#define KEYBOARD_NUM 2


// AIN components
#define KEYBOARD_AIN_NUM        3
#define KEYBOARD_AIN_PITCHWHEEL 0
#define KEYBOARD_AIN_MODWHEEL   1
#define KEYBOARD_AIN_SUSTAIN    2


// send a note over an external hook
// optional (demo)
// usage: see $MIOS32_PATH/apps/controllers/midibox_ng_v1
#ifndef KEYBOARD_NOTIFY_TOGGLE_HOOK
//#define KEYBOARD_NOTIFY_TOGGLE_HOOK
#endif

// optionally disable MIDI config (chn/ports)
#ifndef KEYBOARD_DONT_USE_MIDI_CFG
#define KEYBOARD_DONT_USE_MIDI_CFG 0
#endif

// optionally disable AINs
#ifndef KEYBOARD_DONT_USE_AIN
#define KEYBOARD_DONT_USE_AIN 0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef struct {
#if !KEYBOARD_DONT_USE_MIDI_CFG
  u16 midi_ports;
  u8  midi_chn;
#endif

  u8  note_offset;

  u8  num_rows;
  u8  selected_row;
  u8  prev_row;
  u8  verbose_level;

  u8  dout_sr1;
  u8  dout_sr2;
  u8  din_sr1;
  u8  din_sr2;
  u8  din_key_offset;

  u8  din_inverted:1;
  u8  break_inverted:1;
  u8  scan_velocity:1;
  u8  scan_optimized:1;
  u8  scan_release_velocity:1;
  u8  make_debounced:1;
  u8  break_is_make:1;

  u16 delay_fastest;
  u16 delay_fastest_black_keys;
  u16 delay_fastest_release;
  u16 delay_fastest_release_black_keys;
  u16 delay_slowest;
  u16 delay_slowest_release;

#if !KEYBOARD_DONT_USE_AIN
  u32 ain_timestamp[KEYBOARD_AIN_NUM];
  u8  ain_pin[KEYBOARD_AIN_NUM];
  u8  ain_ctrl[KEYBOARD_AIN_NUM];
  u8  ain_min[KEYBOARD_AIN_NUM];
  u8  ain_max[KEYBOARD_AIN_NUM];
  u8  ain_last_value7[KEYBOARD_AIN_NUM];
  u8  ain_inverted[KEYBOARD_AIN_NUM];
  u8  ain_sustain_switch;
  u8  ain_bandwidth_ms;
#endif
} keyboard_config_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 KEYBOARD_Init(u32 mode);

extern s32 KEYBOARD_ConnectedNumSet(u8 num);
extern u8  KEYBOARD_ConnectedNumGet(void);

extern void KEYBOARD_SRIO_ServicePrepare(void);
extern void KEYBOARD_SRIO_ServiceFinish(void);
extern void KEYBOARD_Periodic_1mS(void);

#if !KEYBOARD_DONT_USE_AIN
extern void KEYBOARD_AIN_NotifyChange(u32 pin, u32 pin_value);
#endif

extern s32 KEYBOARD_TerminalHelp(void *_output_function);
extern s32 KEYBOARD_TerminalParseLine(char *input, void *_output_function);
extern s32 KEYBOARD_TerminalPrintConfig(int kb, void *_output_function);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern keyboard_config_t keyboard_config[KEYBOARD_NUM];


#endif /* _KEYBOARD_H */
