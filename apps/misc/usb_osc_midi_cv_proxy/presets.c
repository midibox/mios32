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

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <eeprom.h>
#include "presets.h"
#include "midimon.h"
#include "uip_task.h"
#include "osc_server.h"


/////////////////////////////////////////////////////////////////////////////
// Reads the EEPROM content during boot
// If EEPROM content isn't valid (magic number mismatch), clear EEPROM
// with default data
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Init(u32 mode)
{
  s32 status = 0;

  // init EEPROM emulation
  if( (status=EEPROM_Init(0)) < 0 ) {
    DEBUG_MSG("[PRESETS] EEPROM initialisation failed with status %d!\n", status);
    return status;
  }

  // check for magic number in EEPROM - if not available, initialize the structure
  if( PRESETS_Read32(PRESETS_ADDR_MAGIC01) != EEPROM_MAGIC_NUMBER ) {
    DEBUG_MSG("[PRESETS] magic number not found - initialize EEPROM!\n");

    // clear EEPROM
    int addr;
    for(addr=0; addr<PRESETS_EEPROM_SIZE; ++addr)
      status |= PRESETS_Write16(addr, 0x00);

    status |= PRESETS_StoreAll();

    // finally write magic numbers
    if( status >= 0 )
      status |= PRESETS_Write32(PRESETS_ADDR_MAGIC01, EEPROM_MAGIC_NUMBER);
  }

  if( status >= 0 ) {
    DEBUG_MSG("[PRESETS] reading configuration data from EEPROM!\n");

    u8 midimon_setup = PRESETS_Read16(PRESETS_ADDR_MIDIMON);
    status |= MIDIMON_InitFromPresets((midimon_setup>>0) & 1, (midimon_setup>>1) & 1, (midimon_setup>>2) & 1);

    status |= UIP_TASK_InitFromPresets(PRESETS_Read16(PRESETS_ADDR_UIP_USE_DHCP),
				       PRESETS_Read32(PRESETS_ADDR_UIP_IP01),
				       PRESETS_Read32(PRESETS_ADDR_UIP_NETMASK01),
				       PRESETS_Read32(PRESETS_ADDR_UIP_GATEWAY01));

    status |= OSC_SERVER_InitFromPresets(PRESETS_Read32(PRESETS_ADDR_OSC_REMOTE01),
					 PRESETS_Read16(PRESETS_ADDR_OSC_REMOTE_PORT),
					 PRESETS_Read16(PRESETS_ADDR_OSC_LOCAL_PORT));
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help functions to read a value from EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
u16 PRESETS_Read16(u8 addr)
{
  return EEPROM_Read(addr);
}

u32 PRESETS_Read32(u8 addr)
{
  return ((u32)EEPROM_Read(addr) << 16) | EEPROM_Read(addr+1);
}

/////////////////////////////////////////////////////////////////////////////
// Help function to write a value into EEPROM (big endian coding!)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_Write16(u8 addr, u16 value)
{
  return EEPROM_Write(addr, value);
}

s32 PRESETS_Write32(u8 addr, u32 value)
{
  return EEPROM_Write(addr, (value >> 16) & 0xffff) | EEPROM_Write(addr+1, (value >> 0) & 0xffff);
}


/////////////////////////////////////////////////////////////////////////////
// Stores all presets
// (the EEPROM emulation ensures that a value won't be written if it is already
// equal)
/////////////////////////////////////////////////////////////////////////////
s32 PRESETS_StoreAll(void)
{
  s32 status = 0;

  // write MIDImon data
  status |= PRESETS_Write16(PRESETS_ADDR_MIDIMON,
			    (MIDIMON_ActiveGet() ? 1 : 0) |
			    (MIDIMON_FilterActiveGet() ? 2 : 0) |
			    (MIDIMON_TempoActiveGet() ? 4 : 0));

  // write uIP data
  status |= PRESETS_Write16(PRESETS_ADDR_UIP_USE_DHCP, UIP_TASK_DHCP_EnableGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_IP01, UIP_TASK_IP_AddressGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_NETMASK01, UIP_TASK_NetmaskGet());
  status |= PRESETS_Write32(PRESETS_ADDR_UIP_GATEWAY01, UIP_TASK_GatewayGet());

  // write OSC data
  status |= PRESETS_Write32(PRESETS_ADDR_OSC_REMOTE01, OSC_SERVER_RemoteIP_Get());
  status |= PRESETS_Write16(PRESETS_ADDR_OSC_REMOTE_PORT, OSC_SERVER_RemotePortGet());
  status |= PRESETS_Write16(PRESETS_ADDR_OSC_LOCAL_PORT, OSC_SERVER_LocalPortGet());

  return 0; // no error
}

