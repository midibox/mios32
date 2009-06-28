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

#include "seq_ui.h"


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

extern s32 SEQ_MIDI_SYSEX_REMOTE_SendMode(seq_ui_remote_mode_t mode);
extern s32 SEQ_MIDI_SYSEX_REMOTE_SendRefresh(void);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendButton(u8 pin, u8 depressed);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_Send_BLM_Button(u8 row, u8 pin, u8 depressed);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Client_SendEncoder(u8 encoder, s8 incrementer);

extern s32 SEQ_MIDI_SYSEX_REMOTE_Server_SendLCD(u8 x, u8 y, u8 *str, u8 len);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Server_SendCharset(u8 charset);
extern s32 SEQ_MIDI_SYSEX_REMOTE_Server_SendLED(u8 first_sr, u8 *led_sr, u8 num_sr);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_MIDI_SYSEX_H */
