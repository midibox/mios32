// $Id$
/*
 * Patch Layer
 * see README.txt for details
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

#include "patch.h"


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// the patch structure (could also be located somewhere else, depending on
// where and how you are storing values in RAM)
u8 patch_structure[256];


/////////////////////////////////////////////////////////////////////////////
// This function initializes the patch structure
/////////////////////////////////////////////////////////////////////////////
s32 PATCH_Init(u32 mode)
{
  if( mode != 0 )
    return -1; // only mode 0 supported

  // init EEPROM emulation
  EEPROM_Init(0);

  // load first patch
  PATCH_Load(0, 0); // bank, patch

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns a byte from patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
u8 PATCH_ReadByte(u8 addr)
{
  return patch_structure[addr];
}


/////////////////////////////////////////////////////////////////////////////
// This function writes a byte into patch structure in RAM
/////////////////////////////////////////////////////////////////////////////
s32 PATCH_WriteByte(u8 addr, u8 byte)
{
  patch_structure[addr] = byte;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function loads the patch structure from EEPROM/BankStick
// Returns != 0 if Load failed (e.g. BankStick not connected)
/////////////////////////////////////////////////////////////////////////////
s32 PATCH_Load(u8 bank, u8 patch)
{
  s32 status;
  int i;

#if PATCH_USE_BANKSTICK
  // determine offset depending on patch number
  u16 offset = patch << 8;

  // use 64byte page load functions for faster access
  // TODO: proper error and retry handling
  for(i=0; i<4; ++i)
    if( status = MIOS32_IIC_BS_Read(bank, offset + i*0x40, (u8 *)(patch_structure + i*0x40), 0x40) ) {
      return status;
    }
#else
  // EEPROM Emulation
  if( bank > 0 )
    return -1; // only bank 0 supported

  if( patch > 0 )
    return -1; // only a single patch is supported

  status = 0;
  for(i=0; i<128; i+=2) {
    s32 value = EEPROM_Read(i/2);

    if( value < 0 )
      value = 0; // valid not programmed yet
    
    patch_structure[i+0] = (value >> 0) & 0xff;
    patch_structure[i+1] = (value >> 8) & 0xff;
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch structure into EEPROM/BankStick
// Returns != 0 if Store failed (e.g. BankStick not connected)
/////////////////////////////////////////////////////////////////////////////
s32 PATCH_Store(u8 bank, u8 patch)
{
  s32 status;
  int i;

#if PATCH_USE_BANKSTICK
  // determine offset depending on patch number
  u16 offset = patch << 8;

  // use 64byte page write functions for faster access
  // TODO: proper error and retry handling
  for(i=0; i<4; ++i) {
    if( status = MIOS32_IIC_BS_Write(bank, offset + i*0x40, (u8 *)(patch_structure + i*0x40), 0x40) ) {
      return status;
    }
    while( status = MIOS32_IIC_BS_CheckWriteFinished(bank) ) {
      // for IIC debugging
      if( status < 0 ) { // returns <0 on error, returns 1 if write operation in progress (ok)
	return status;
      }
    }
  }
#else
  // EEPROM Emulation
  if( bank > 0 )
    return -1; // only bank 0 supported

  if( patch > 0 )
    return -1; // only a single patch is supported

  for(i=0; i<128; i+=2) {
    u16 hword = patch_structure[i+0] | (patch_structure[i+1] << 8);

    if( (status=EEPROM_Write(i/2, hword)) < 0 )
      return status; // error
  }
#endif

  return 0; // no error
}
