// $Id$
/*
 * Header file for SysEx routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_SYSEX_H
#define _SID_SYSEX_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// ASID Mode
typedef enum {
  SID_SYSEX_ASID_MODE_OFF,
  SID_SYSEX_ASID_MODE_ON
} sid_sysex_asid_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_SYSEX_Init(u32 mode);

extern s32 SID_SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);
extern s32 SID_SYSEX_TimeOut(mios32_midi_port_t port);

extern sid_sysex_asid_mode_t SID_SYSEX_ASID_ModeGet(void);
extern s32 SID_SYSEX_ASID_ModeSet(sid_sysex_asid_mode_t mode);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SID_SYSEX_H */
