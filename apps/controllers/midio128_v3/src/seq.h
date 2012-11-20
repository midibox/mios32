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

#define SEQ_MIDI_PLAY_MODE_NUM    2 // two available modes

#define SEQ_MIDI_PLAY_MODE_ALL    0
#define SEQ_MIDI_PLAY_MODE_SINGLE 1

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_Init(u32 mode);

extern s32 SEQ_Reset(u8 play_off_events);
extern s32 SEQ_Handler(void);

extern s32 SEQ_PlayFileReq(s8 next, u8 force);

extern s32 SEQ_MidiRecReceive(mios32_midi_port_t port, mios32_midi_package_t package);

extern s32 SEQ_PauseEnabled(void);
extern s32 SEQ_RecordingEnabled(void);
extern char *SEQ_RecordingFilename(void);
extern u32 SEQ_LastRecordedTick(void);
extern mios32_midi_port_t SEQ_LastRecordedPort(void);
extern mios32_midi_package_t SEQ_LastRecordedEvent(void);

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


#endif /* _SEQ_H */
