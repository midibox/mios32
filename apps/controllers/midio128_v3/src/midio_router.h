// $Id$
/*
 * MIDI Router functions for MIDIO128 V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO_ROUTER_H
#define _MIDIO_ROUTER_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIO_ROUTER_Init(u32 mode);

extern s32 MIDIO_ROUTER_Receive(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 MIDIO_ROUTER_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in);

extern s32 MIDIO_ROUTER_MIDIClockInGet(mios32_midi_port_t port);
extern s32 MIDIO_ROUTER_MIDIClockInSet(mios32_midi_port_t port, u8 enable);

extern s32 MIDIO_ROUTER_MIDIClockOutGet(mios32_midi_port_t port);
extern s32 MIDIO_ROUTER_MIDIClockOutSet(mios32_midi_port_t port, u8 enable);

extern s32 MIDIO_ROUTER_SendMIDIClockEvent(u8 evnt0, u32 bpm_tick);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIDIO_ROUTER_H */
