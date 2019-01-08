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

// BLM faders
#define SEQ_BLM_NUM_FADERS 8


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 ALWAYS_USE_FTS:1;  // Force-to-Scale always used independent from the Track Mode selection
    u8 SWAP_KEYBOARD_COLOURS:1;  // swap LED colours in keyboard display
  };
} seq_blm_options_t;


typedef union {
  u32 ALL;
  struct {
    u8 port;
    u8 chn;
    u16 send_function;
  };
} seq_blm_fader_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_BLM_Init(u32 mode);

extern s32 SEQ_BLM_LED_Update(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mios32_midi_port_t seq_blm_port;
extern u8 seq_blm_timeout_ctr;
extern seq_blm_options_t seq_blm_options;
extern seq_blm_fader_t seq_blm_fader[SEQ_BLM_NUM_FADERS];

#endif /* _SEQ_MIDI_BLM_H */
