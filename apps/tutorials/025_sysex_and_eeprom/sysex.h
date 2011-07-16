// $Id$
/*
 * SysEx Parser Demo
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYSEX_H
#define _SYSEX_H


/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////

// use checksum protection?
// (disadvantage: files cannot be edited without re-calculating the checksum, therefore disabled by default)
#define SYSEX_CHECKSUM_PROTECTION 0


// 0: EEPROM content sent/received as 7bit values (8th bit discarded)
// 1: EEPROM content sent/received as 2 x 4bit values (doubles the dump size)
#define SYSEX_FORMAT  0


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SYSEX_Init(u32 mode);
extern s32 SYSEX_Send(mios32_midi_port_t port, u8 bank, u8 patch);
extern s32 SYSEX_Parser(mios32_midi_port_t port, u8 midi_in);


/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SYSEX_H */
