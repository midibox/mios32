// $Id$
/*
 * Header file for MIDI SysEx routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_SYSEX_H
#define _SEQ_MIDI_SYSEX_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_SYSEX_Init(u32 mode);

extern s32 SEQ_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SEQ_MIDI_SYSEX_TimeOut(mios32_midi_port_t port);

extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendOnOffRequest(u8 on);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendButton(u8 pin, u8 depressed);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_Send_BLM_Button(u8 row, u8 pin, u8 depressed);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendEncoder(u8 encoder, s8 incrementer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_MIDI_SYSEX_H */
