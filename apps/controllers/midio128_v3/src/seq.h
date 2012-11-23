// $Id$
/*
 * Header file for sequencer routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_H
#define _SEQ_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_MIDI_PLAY_MODE_NUM    3 // three available modes

#define SEQ_MIDI_PLAY_MODE_ALL         0
#define SEQ_MIDI_PLAY_MODE_SINGLE      1
#define SEQ_MIDI_PLAY_MODE_SINGLE_LOOP 2

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_Init(u32 mode);

extern u8 SEQ_ClockModeGet(void);
extern s32 SEQ_ClockModeSet(u8 mode);

extern s32 SEQ_Reset(u8 play_off_events);
extern s32 SEQ_Handler(void);

extern s32 SEQ_PlayFileReq(s8 next, u8 force);

extern s32 SEQ_PauseEnabled(void);
extern s32 SEQ_SetPauseMode(u8 enable);

extern s32 SEQ_PlayStopButton(void);
extern s32 SEQ_RecStopButton(void);
extern s32 SEQ_FFwdButton(void);
extern s32 SEQ_FRewButton(void);

extern s32 SEQ_PlayFile(char *midifile);

extern s32 SEQ_MidiPlayModeGet(void);
extern s32 SEQ_MidiPlayModeSet(u8 mode);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u16 seq_play_enabled_ports;
extern u16 seq_rec_enabled_ports;

extern u8 seq_play_enable_dout;
extern u8 seq_rec_enable_din;

#endif /* _SEQ_H */
