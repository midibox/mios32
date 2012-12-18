// $Id$
/*
 * Motorfader access functions for MIDIbox NG
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MBNG_MF_H
#define _MBNG_MF_H

#include "mbng_event.h"

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MBNG_MF_Init(u32 mode);

extern s32 MBNG_MF_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package);
extern s32 MBNG_MF_ReceiveSysEx(mios32_midi_port_t port, u8 midi_in);

extern s32 MBNG_MF_NotifyReceivedValue(mbng_event_item_t *item, u16 value);
extern s32 MBNG_MF_GetCurrentValueFromId(mbng_event_item_id_t id);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MBNG_MF_H */
