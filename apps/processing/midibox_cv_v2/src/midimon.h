// $Id$
/*
 * Header file for MIDI monitor module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIMON_H
#define _MIDIMON_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIMON_Init(u32 mode);
extern s32 MIDIMON_InitFromPresets(u8 _midimon_active, u8 _filter_active, u8 _tempo_active);

extern s32 MIDIMON_Receive(mios32_midi_port_t port, mios32_midi_package_t package, u32 timestamp, u8 filter_sysex_message);

extern s32 MIDIMON_ActiveSet(u8 active);
extern s32 MIDIMON_ActiveGet(void);
extern s32 MIDIMON_FilterActiveSet(u8 active);
extern s32 MIDIMON_FilterActiveGet(void);
extern s32 MIDIMON_TempoActiveSet(u8 active);
extern s32 MIDIMON_TempoActiveGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIDIMON_H */
