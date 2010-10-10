// $Id$
/*
 * Header file for OSC client functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _OSC_CLIENT_H
#define _OSC_CLIENT_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define OSC_CLIENT_NUM_PORTS 4


// transfer modes
#define OSC_CLIENT_NUM_TRANSFER_MODES 4

#define OSC_CLIENT_TRANSFER_MODE_MIDI  0
#define OSC_CLIENT_TRANSFER_MODE_INT   1
#define OSC_CLIENT_TRANSFER_MODE_FLOAT 2
#define OSC_CLIENT_TRANSFER_MODE_MCMPP 3


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 OSC_CLIENT_Init(u32 mode);

extern s32 SEQ_MIDI_OSC_TransferModeSet(u8 osc_port, u8 mode);
extern u8 SEQ_MIDI_OSC_TransferModeGet(u8 osc_port);

extern s32 OSC_CLIENT_SendMIDIEvent(u8 osc_port, mios32_midi_package_t p);
extern s32 OSC_CLIENT_SendSysEx(u8 osc_port, u8 *stream, u32 count);
extern s32 OSC_CLIENT_SendMIDIEventBundled(u8 osc_port, mios32_midi_package_t *p, u8 num_events, mios32_osc_timetag_t timetag);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _OSC_CLIENT_H */
