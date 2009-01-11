// $Id$
/*
 * Header file for MMC/SD Card Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SDCARD_H
#define _MIOS32_SDCARD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// Which SPI peripheral should be used
// allowed values: 1 and 2
// (note: SPI1 will allocate DMA channel 2 and 3, SPI2 will allocate DMA channel 4 and 5)
#ifndef MIOS32_SDCARD_SPI
#define MIOS32_SDCARD_SPI 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SDCARD_Init(u32 mode);

extern s32 MIOS32_SDCARD_PowerOn(void);
extern s32 MIOS32_SDCARD_PowerOff(void);
extern s32 MIOS32_SDCARD_CheckAvailable(u8 was_available);

extern s32 MIOS32_SDCARD_SendSDCCmd(u8 cmd, u32 addr, u8 crc);
extern s32 MIOS32_SDCARD_SectorRead(u32 sector, u8 *buffer);
extern s32 MIOS32_SDCARD_SectorWrite(u32 sector, u8 *buffer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_SDCARD_H */
