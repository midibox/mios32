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

// structure taken from Mass Storage Driver example provided by STM
typedef struct
{
  u8  CSDStruct;            /* CSD structure */
  u8  SysSpecVersion;       /* System specification version */
  u8  Reserved1;            /* Reserved */
  u8  TAAC;                 /* Data read access-time 1 */
  u8  NSAC;                 /* Data read access-time 2 in CLK cycles */
  u8  MaxBusClkFrec;        /* Max. bus clock frequency */
  u16 CardComdClasses;      /* Card command classes */
  u8  RdBlockLen;           /* Max. read data block length */
  u8  PartBlockRead;        /* Partial blocks for read allowed */
  u8  WrBlockMisalign;      /* Write block misalignment */
  u8  RdBlockMisalign;      /* Read block misalignment */
  u8  DSRImpl;              /* DSR implemented */
  u8  Reserved2;            /* Reserved */
  u32 DeviceSize;           /* Device Size */
  u8  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
  u8  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
  u8  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
  u8  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
  u8  DeviceSizeMul;        /* Device size multiplier */
  u8  EraseGrSize;          /* Erase group size */
  u8  EraseGrMul;           /* Erase group size multiplier */
  u8  WrProtectGrSize;      /* Write protect group size */
  u8  WrProtectGrEnable;    /* Write protect group enable */
  u8  ManDeflECC;           /* Manufacturer default ECC */
  u8  WrSpeedFact;          /* Write speed factor */
  u8  MaxWrBlockLen;        /* Max. write data block length */
  u8  WriteBlockPaPartial;  /* Partial blocks for write allowed */
  u8  Reserved3;            /* Reserded */
  u8  ContentProtectAppli;  /* Content protection application */
  u8  FileFormatGrouop;     /* File format group */
  u8  CopyFlag;             /* Copy flag (OTP) */
  u8  PermWrProtect;        /* Permanent write protection */
  u8  TempWrProtect;        /* Temporary write protection */
  u8  FileFormat;           /* File Format */
  u8  ECC;                  /* ECC code */
  u8  msd_CRC;                  /* CRC */
  u8  Reserved4;            /* always 1*/
} mios32_sdcard_csd_t;

// structure taken from Mass Storage Driver example provided by STM
typedef struct
{
  u8  ManufacturerID;       /* ManufacturerID */
  u16 OEM_AppliID;          /* OEM/Application ID */
  char ProdName[6];         /* Product Name */
  u8  ProdRev;              /* Product Revision */
  u32 ProdSN;               /* Product Serial Number */
  u8  Reserved1;            /* Reserved1 */
  u16 ManufactDate;         /* Manufacturing Date */
  u8  msd_CRC;              /* CRC */
  u8  Reserved2;            /* always 1*/
} mios32_sdcard_cid_t;


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

extern s32 MIOS32_SDCARD_CIDRead(mios32_sdcard_cid_t *cid);
extern s32 MIOS32_SDCARD_CSDRead(mios32_sdcard_csd_t *csd);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_SDCARD_H */
