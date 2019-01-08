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

typedef enum {
  SEQ_MIDI_SYSEX_REMOTE_MODE_AUTO=0,
  SEQ_MIDI_SYSEX_REMOTE_MODE_SERVER,
  SEQ_MIDI_SYSEX_REMOTE_MODE_CLIENT
} seq_midi_sysex_remote_mode_t;



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_SYSEX_Init(u32 mode);

extern s32 SEQ_MIDI_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SEQ_MIDI_SYSEX_TimeOut(mios32_midi_port_t port);

extern s32 SEQ_MIDI_SYSEX_REMOTE_SendMode(seq_midi_sysex_remote_mode_t mode);
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

extern seq_midi_sysex_remote_mode_t seq_midi_sysex_remote_mode;
extern seq_midi_sysex_remote_mode_t seq_midi_sysex_remote_active_mode;
extern mios32_midi_port_t seq_midi_sysex_remote_port;
extern mios32_midi_port_t seq_midi_sysex_remote_active_port;
extern u8 seq_midi_sysex_remote_id;
extern u16 seq_midi_sysex_remote_client_timeout_ctr;
extern u8 seq_midi_sysex_remote_force_lcd_update;
extern u8 seq_midi_sysex_remote_force_led_update;


#endif /* _SEQ_MIDI_SYSEX_H */
