// $Id$
/*
 * Preset handling
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _PRESETS_H
#define _PRESETS_H

#include <eeprom.h>

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// EEPROM locations
#define PRESETS_EEPROM_SIZE    EEPROM_EMULATED_SIZE // prepared for up to 128 entries

#define PRESETS_ADDR_RESERVED  0x00 // should not be written
#define PRESETS_ADDR_MAGIC01   0x01
#define PRESETS_ADDR_MAGIC23   0x02

#define PRESETS_ADDR_MIDIMON   0x04

#define PRESETS_ADDR_UIP_USE_DHCP  0x08
#define PRESETS_ADDR_UIP_IP01      0x12
#define PRESETS_ADDR_UIP_IP23      0x13
#define PRESETS_ADDR_UIP_NETMASK01 0x14
#define PRESETS_ADDR_UIP_NETMASK23 0x15
#define PRESETS_ADDR_UIP_GATEWAY01 0x16
#define PRESETS_ADDR_UIP_GATEWAY23 0x17

#define PRESETS_ADDR_OSC_REMOTE01  0x20
#define PRESETS_ADDR_OSC_REMOTE23  0x21
#define PRESETS_ADDR_OSC_PORT      0x22

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 PRESETS_Init(u32 mode);

extern u16 PRESETS_Read16(u8 addr);
extern u32 PRESETS_Read32(u8 addr);

extern s32 PRESETS_Write16(u8 addr, u16 value);
extern s32 PRESETS_Write32(u8 addr, u32 value);

extern s32 PRESETS_StoreAll(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _PRESETS_H */
