// $Id$
/*
 * Header file for MIDI Input routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_IN_H
#define _SEQ_MIDI_IN_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_IN_Init(u32 mode);

extern s32 SEQ_MIDI_IN_ResetNoteStacks(void);

extern s32 SEQ_MIDI_IN_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);

extern s32 SEQ_MIDI_IN_TransposerNoteGet(u8 hold);
extern s32 SEQ_MIDI_IN_ArpNoteGet(u8 hold, u8 sorted, u8 key_num);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 seq_midi_in_channel;
extern mios32_midi_port_t seq_midi_in_port;
extern mios32_midi_port_t seq_midi_in_mclk_port;
extern u8 seq_midi_in_ta_split_note;


#endif /* _SEQ_MIDI_IN_H */
