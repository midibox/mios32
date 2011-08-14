// $Id$
/*
 * Header file for MIDI OSC routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDI_OSC_H
#define _SEQ_MIDI_OSC_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_MIDI_OSC_NUM_PORTS 4


// transfer modes
#define SEQ_MIDI_OSC_NUM_TRANSFER_MODES 5

#define SEQ_MIDI_OSC_TRANSFER_MODE_MIDI  0
#define SEQ_MIDI_OSC_TRANSFER_MODE_INT   1
#define SEQ_MIDI_OSC_TRANSFER_MODE_FLOAT 2
#define SEQ_MIDI_OSC_TRANSFER_MODE_MCMPP 3
#define SEQ_MIDI_OSC_TRANSFER_MODE_TOSC  4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDI_OSC_Init(u32 mode);

extern s32 SEQ_MIDI_OSC_TransferModeSet(u8 osc_port, u8 mode);
extern u8 SEQ_MIDI_OSC_TransferModeGet(u8 osc_port);

extern s32 SEQ_MIDI_OSC_SendPackage(u8 osc_port, mios32_midi_package_t package);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_MIDI_OSC_H */
