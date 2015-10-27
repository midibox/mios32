// $Id$
/*
 * Header file for MBHP_BLM_SCALAR routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_BLM_H
#define _SEQ_BLM_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// for 16x16 LEDs
// rows should match with number of tracks
#define SEQ_BLM_NUM_ROWS 16

// columns should match with bitwidth of seq_blm_led* variables
// Note: previously we supported wide screen displays (e.g. with 16, 32, 64, 128 LEDs per row)
// this has been removed to simplify and speed up the routines!
// However, a 32 LED mode is still feasible with some changes... have fun! ;)
#define SEQ_BLM_NUM_COLUMNS 16

// BLM faders
#define SEQ_BLM_NUM_FADERS 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 ALWAYS_USE_FTS:1;  // Force-to-Scale always used independent from the Track Mode selection
  };
} seq_blm_options_t;


typedef union {
  u32 ALL;
  struct {
    u8 port;
    u8 chn;
    u8 send_function;
  };
} seq_blm_fader_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_BLM_Init(u32 mode);

extern u16 SEQ_BLM_PatternGreenGet(u8 row);
extern u16 SEQ_BLM_PatternRedGet(u8 row);

extern s32 SEQ_BLM_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);

extern s32 SEQ_BLM_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SEQ_BLM_TimeOut(mios32_midi_port_t port);
extern s32 SEQ_BLM_SYSEX_SendRequest(u8 req);

extern s32 SEQ_BLM_LED_Update(void);
extern s32 SEQ_BLM_MIDI_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mios32_midi_port_t seq_blm_port;
extern u8 seq_blm_timeout_ctr;
extern seq_blm_options_t seq_blm_options;
extern seq_blm_fader_t seq_blm_fader[SEQ_BLM_NUM_FADERS];

#endif /* _SEQ_MIDI_BLM_H */
