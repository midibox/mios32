// $Id$
//!
//! Reduced version of SD Card Block device
//! most functions already handled by MIOS32, therefore only block read/write
//! functions have to be supported
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "blockdev.h"

u16 msd_memory_rd_led_ctr;
u16 msd_memory_wr_led_ctr;


/* ****************************************************************************
 calculates size of card from CSD 
 (extension by Martin Thomas, inspired by code from Holger Klabunde)
 */
int BlockDevGetSize(u32 *pdwDriveSize)
{
    mios32_sdcard_csd_t csd;
    if( MIOS32_SDCARD_CSDRead(&csd) < 0 )
      return 1;

    u32 DeviceSizeMul = csd.DeviceSizeMul + 2;
    u32 temp_block_mul = (1 << csd.RdBlockLen)/ 512;
    u32 count = ((csd.DeviceSize + 1) * (1 << (DeviceSizeMul))) * temp_block_mul;
    u32 block_size = 512;
    *pdwDriveSize = count * block_size;
    return 0;
}

int BlockDevInit(void)
{
  return 0; // nothing to do... device initialized by application
}

int BlockDevWrite(u32 dwAddress, u8 * pbBuf)
{
  msd_memory_wr_led_ctr = 0;
  return MIOS32_SDCARD_SectorWrite(dwAddress, (u8 *)pbBuf);
}

int BlockDevRead(u32 dwAddress, u8 * pbBuf)
{
  msd_memory_rd_led_ctr = 0;
  return MIOS32_SDCARD_SectorRead(dwAddress, (u8 *)pbBuf);
}
