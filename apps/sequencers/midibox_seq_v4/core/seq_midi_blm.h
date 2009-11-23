// $Id$
/*
 * Header file for MBHP_BLM_SCALAR access
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_BLM_H
#define _SEQ_MIDI_BLM_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_MIDI_BLM_MODE_TRIGGERS,
  SEQ_MIDI_BLM_MODE_TRACKS,
  SEQ_MIDI_BLM_MODE_PATTERNS,
} seq_midi_blm_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_BLM_Init(u32 mode);

extern s32 SEQ_MIDI_BLM_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SEQ_MIDI_BLM_TimeOut(mios32_midi_port_t port);
extern s32 SEQ_MIDI_BLM_SYSEX_SendRequest(u8 req);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mios32_midi_port_t seq_midi_blm_port;
extern seq_midi_blm_mode_t seq_midi_blm_mode;
extern u8 seq_midi_blm_num_steps;
extern u8 seq_midi_blm_num_tracks;
extern u8 seq_midi_blm_num_colours;
extern u8 seq_midi_blm_force_update;

#endif /* _SEQ_MIDI_BLM_H */
