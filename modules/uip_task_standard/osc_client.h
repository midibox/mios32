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
// keep OSC_CLIENT_TransferModeFullNameGet() and OSC_CLIENT_TransferModeShortNameGet() aligned with the assignments!
#define OSC_CLIENT_NUM_TRANSFER_MODES 5

#define OSC_CLIENT_TRANSFER_MODE_MIDI  0
#define OSC_CLIENT_TRANSFER_MODE_INT   1
#define OSC_CLIENT_TRANSFER_MODE_FLOAT 2
#define OSC_CLIENT_TRANSFER_MODE_MCMPP 3
#define OSC_CLIENT_TRANSFER_MODE_TOSC  4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 OSC_CLIENT_Init(u32 mode);

extern s32 OSC_CLIENT_TransferModeSet(u8 osc_port, u8 mode);
extern u8 OSC_CLIENT_TransferModeGet(u8 osc_port);

extern const char* OSC_CLIENT_TransferModeFullNameGet(u8 mode);
extern const char* OSC_CLIENT_TransferModeShortNameGet(u8 mode);

extern s32 OSC_CLIENT_SendMIDIEvent(u8 osc_port, mios32_midi_package_t p);
extern s32 OSC_CLIENT_SendSysEx(u8 osc_port, u8 *stream, u32 count);
extern s32 OSC_CLIENT_SendMIDIEventBundled(u8 osc_port, mios32_midi_package_t *p, u8 num_events, mios32_osc_timetag_t timetag);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _OSC_CLIENT_H */
