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
#define PRESETS_EEPROM_SIZE    EEPROM_EMULATED_SIZE // prepared for up to 256 entries

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

#define PRESETS_NUM_OSC_RECORDS            4
#define PRESETS_OFFSET_BETWEEN_OSC_RECORDS 8
#define PRESETS_ADDR_OSC0_REMOTE01    (0x20 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x20
#define PRESETS_ADDR_OSC0_REMOTE23    (0x21 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x21
#define PRESETS_ADDR_OSC0_REMOTE_PORT (0x22 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x22
#define PRESETS_ADDR_OSC0_LOCAL_PORT  (0x23 + 0*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x23

#define PRESETS_ADDR_OSC1_REMOTE01    (0x20 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x28
#define PRESETS_ADDR_OSC1_REMOTE23    (0x21 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x29
#define PRESETS_ADDR_OSC1_REMOTE_PORT (0x22 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x2a
#define PRESETS_ADDR_OSC1_LOCAL_PORT  (0x23 + 1*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x2b

#define PRESETS_ADDR_OSC2_REMOTE01    (0x20 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x30
#define PRESETS_ADDR_OSC2_REMOTE23    (0x21 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x31
#define PRESETS_ADDR_OSC2_REMOTE_PORT (0x22 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x32
#define PRESETS_ADDR_OSC2_LOCAL_PORT  (0x23 + 2*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x33

#define PRESETS_ADDR_OSC3_REMOTE01    (0x20 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x38
#define PRESETS_ADDR_OSC3_REMOTE23    (0x21 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x39
#define PRESETS_ADDR_OSC3_REMOTE_PORT (0x22 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x3a
#define PRESETS_ADDR_OSC3_LOCAL_PORT  (0x23 + 3*PRESETS_OFFSET_BETWEEN_OSC_RECORDS) // 0x3b


#define PRESETS_ADDR_SRIO_NUM          0x80


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
