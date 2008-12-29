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
#if PATCH_USE_BANKSTICK
  // determine offset depending on patch number
  u16 offset = patch << 8;
  s32 error;
  int i;

  // use 64byte page load functions for faster access
  // TODO: proper error and retry handling
  for(i=0; i<4; ++i)
    if( error = MIOS32_IIC_BS_Read(bank, offset + i*0x40, (u8 *)(patch_structure + i*0x40), 0x40) ) {
      return error;
    }
#else
# error "only BankStick supported!"
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function stores the patch structure into EEPROM/BankStick
// Returns != 0 if Store failed (e.g. BankStick not connected)
/////////////////////////////////////////////////////////////////////////////
s32 PATCH_Store(u8 bank, u8 patch)
{
#if PATCH_USE_BANKSTICK
  // determine offset depending on patch number
  u16 offset = patch << 8;
  s32 error;
  int i;

  // use 64byte page write functions for faster access
  // TODO: proper error and retry handling
  for(i=0; i<4; ++i) {
    if( error = MIOS32_IIC_BS_Write(bank, offset + i*0x40, (u8 *)(patch_structure + i*0x40), 0x40) ) {
      return error;
    }
    while( error = MIOS32_IIC_BS_CheckWriteFinished(bank) ) {
      // for IIC debugging
      if( error < 0 ) { // returns <0 on error, returns 1 if write operation in progress (ok)
	return error;
      }
    }
#if 1
    // TODO: check why a delay is required after NAK polling!
    volatile u32 delay;
    for(delay=0; delay<100; ++delay);
#endif
  }
#else
# error "only BankStick supported!"
#endif

  return 0; // no error
}
