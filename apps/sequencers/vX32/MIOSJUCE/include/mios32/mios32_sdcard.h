// $Id: mios32_sdcard.h 358 2009-02-21 23:21:17Z tk $
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
// allowed values: 0 and 1
// (note: SPI0 will allocate DMA channel 2 and 3, SPI1 will allocate DMA channel 4 and 5)
#ifndef MIOS32_SDCARD_SPI
#define MIOS32_SDCARD_SPI 0
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC)
#ifndef MIOS32_SDCARD_SPI_RC_PIN
#define MIOS32_SDCARD_SPI_RC_PIN 0
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
