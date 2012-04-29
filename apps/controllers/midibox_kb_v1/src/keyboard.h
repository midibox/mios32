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


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef struct {

  u16 midi_ports;
  u8  midi_chn;

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

  u16 delay_fastest;
  u16 delay_slowest;

  u8  ain_pitchwheel;
  u8  ctrl_pitchwheel;
  u8  ain_modwheel;
  u8  ctrl_modwheel;
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

extern void KEYBOARD_AIN_NotifyChange(u32 pin, u32 pin_value);

extern s32 KEYBOARD_TerminalHelp(void *_output_function);
extern s32 KEYBOARD_TerminalParseLine(char *input, void *_output_function);
extern s32 KEYBOARD_TerminalPrintConfig(int kb, void *_output_function);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern keyboard_config_t keyboard_config[KEYBOARD_NUM];


#endif /* _KEYBOARD_H */
