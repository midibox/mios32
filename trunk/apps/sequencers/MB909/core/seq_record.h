// $Id: seq_record.h 2168 2015-04-27 18:45:02Z tk $
/*
 * Header file for core routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_RECORD_H
#define _SEQ_RECORD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL;
  struct {
    u8 STEPS_PER_KEY;
    u8 STEP_RECORD:1;
    u8 POLY_RECORD:1;
    u8 AUTO_START:1;
    u8 FWD_MIDI:1;
  };
} seq_record_options_t;


typedef union {
  u32 ALL;
  struct {
    u16 ARMED_TRACKS;
    u32 ENABLED:1;
  };
} seq_record_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_RECORD_Init(u32 mode);
extern s32 SEQ_RECORD_Reset(u8 track);
extern s32 SEQ_RECORD_ResetAllTracks(void);
extern s32 SEQ_RECORD_AllNotesOff(void);
extern s32 SEQ_RECORD_Enable(u8 enable, u8 reset_timestamps);

extern s32 SEQ_RECORD_PrintEditScreen(void);

extern s32 SEQ_RECORD_Receive(mios32_midi_package_t midi_package, u8 track);

extern s32 SEQ_RECORD_NewStep(u8 track, u8 prev_step, u8 new_step, u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_record_options_t seq_record_options;
extern seq_record_state_t seq_record_state;

extern u8 seq_record_quantize;

extern u32 seq_record_played_notes[4];

#endif /* _SEQ_RECORD_H */
