// $Id$
/*
 * SD Card Info
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

#include <string.h>
#include <dosfs.h>

#include "app.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
// should be enabled for this application!
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// DOS FS variables
static u8 sector[SECTOR_SIZE];
static VOLINFO vi;
static DIRINFO di;
static DIRENT de;
static FILEINFO fi;


/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);

  MIOS32_SDCARD_Init(0);

#if DEBUG_VERBOSE_LEVEL >= 1
  // print welcome message on MIOS terminal
  DEBUG_MSG("\n");
  DEBUG_MSG("====================\n");
  DEBUG_MSG("%s\n", MIOS32_LCD_BOOT_MSG_LINE1);
  DEBUG_MSG("====================\n");
  DEBUG_MSG("\n");
#endif
}




/////////////////////////////////////////////////////////////////////////////
// This function prints some useful SD card informations
/////////////////////////////////////////////////////////////////////////////
s32 PrintSDCardInfos(void)
{
  int status = 0;

  // read CID data
  mios32_sdcard_cid_t cid;
  if( (status=MIOS32_SDCARD_CIDRead(&cid)) < 0 ) {
    DEBUG_MSG("ERROR: Reading CID failed with status %d!\n", status);
    // continue regardless if we got an error or not...
  } else {
    DEBUG_MSG("--------------------\n");
    DEBUG_MSG("CID:\n");
    DEBUG_MSG("- ManufacturerID:\n", cid.ManufacturerID);
    DEBUG_MSG("- OEM AppliID:\n", cid.OEM_AppliID);
    DEBUG_MSG("- ProdName: %s\n", cid.ProdName);
    DEBUG_MSG("- ProdRev: %u\n", cid.ProdRev);
    DEBUG_MSG("- ProdSN: 0x%08x\n", cid.ProdSN);
    DEBUG_MSG("- Reserved1: %u\n", cid.Reserved1);
    DEBUG_MSG("- ManufactDate: %u\n", cid.ManufactDate);
    DEBUG_MSG("- msd_CRC: 0x%02x\n", cid.msd_CRC);
    DEBUG_MSG("- Reserved2: %u\n", cid.Reserved2);
    DEBUG_MSG("--------------------\n");
  }


  // read CSD data
  mios32_sdcard_csd_t csd;
  if( (status=MIOS32_SDCARD_CSDRead(&csd)) < 0 ) {
    DEBUG_MSG("ERROR: Reading CSD failed with status %d!\n", status);
    // continue regardless if we got an error or not...
  } else {
    DEBUG_MSG("--------------------\n");
    DEBUG_MSG("CSDStruct: %u\n", csd.CSDStruct);
    DEBUG_MSG("SysSpecVersion: %u\n", csd.SysSpecVersion);
    DEBUG_MSG("Reserved1: %u\n", csd.Reserved1);
    DEBUG_MSG("TAAC: %u\n", csd.TAAC);
    DEBUG_MSG("NSAC: %u\n", csd.NSAC);
    DEBUG_MSG("MaxBusClkFrec: %u\n", csd.MaxBusClkFrec);
    DEBUG_MSG("CardComdClasses: %u\n", csd.CardComdClasses);
    DEBUG_MSG("RdBlockLen: %u\n", csd.RdBlockLen);
    DEBUG_MSG("PartBlockRead: %u\n", csd.PartBlockRead);
    DEBUG_MSG("WrBlockMisalign: %u\n", csd.WrBlockMisalign);
    DEBUG_MSG("RdBlockMisalign: %u\n", csd.RdBlockMisalign);
    DEBUG_MSG("DSRImpl: %u\n", csd.DSRImpl);
    DEBUG_MSG("Reserved2: %u\n", csd.Reserved2);
    DEBUG_MSG("DeviceSize: %u\n", csd.DeviceSize);
    DEBUG_MSG("MaxRdCurrentVDDMin: %u\n", csd.MaxRdCurrentVDDMin);
    DEBUG_MSG("MaxRdCurrentVDDMax: %u\n", csd.MaxRdCurrentVDDMax);
    DEBUG_MSG("MaxWrCurrentVDDMin: %u\n", csd.MaxWrCurrentVDDMin);
    DEBUG_MSG("MaxWrCurrentVDDMax: %u\n", csd.MaxWrCurrentVDDMax);
    DEBUG_MSG("DeviceSizeMul: %u\n", csd.DeviceSizeMul);
    DEBUG_MSG("EraseGrSize: %u\n", csd.EraseGrSize);
    DEBUG_MSG("EraseGrMul: %u\n", csd.EraseGrMul);
    DEBUG_MSG("WrProtectGrSize: %u\n", csd.WrProtectGrSize);
    DEBUG_MSG("WrProtectGrEnable: %u\n", csd.WrProtectGrEnable);
    DEBUG_MSG("ManDeflECC: %u\n", csd.ManDeflECC);
    DEBUG_MSG("WrSpeedFact: %u\n", csd.WrSpeedFact);
    DEBUG_MSG("MaxWrBlockLen: %u\n", csd.MaxWrBlockLen);
    DEBUG_MSG("WriteBlockPaPartial: %u\n", csd.WriteBlockPaPartial);
    DEBUG_MSG("Reserved3: %u\n", csd.Reserved3);
    DEBUG_MSG("ContentProtectAppli: %u\n", csd.ContentProtectAppli);
    DEBUG_MSG("FileFormatGrouop: %u\n", csd.FileFormatGrouop);
    DEBUG_MSG("CopyFlag: %u\n", csd.CopyFlag);
    DEBUG_MSG("PermWrProtect: %u\n", csd.PermWrProtect);
    DEBUG_MSG("TempWrProtect: %u\n", csd.TempWrProtect);
    DEBUG_MSG("FileFormat: %u\n", csd.FileFormat);
    DEBUG_MSG("ECC: %u\n", csd.ECC);
    DEBUG_MSG("msd_CRC: 0x%02x\n", csd.msd_CRC);
    DEBUG_MSG("Reserved4: %u\n", csd.Reserved4);
    DEBUG_MSG("--------------------\n");
  }
  
  // try to mount file system
  u32 pstart, psize;
  u8  pactive, ptype;

  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
  if( pstart == 0xffffffff ) {
    DEBUG_MSG("ERROR: Cannot find first partition!\n");
    return -1; // error
  }

  DEBUG_MSG("--------------------\n");
  // check for partition type, if we don't get one of these types, it can be assumed that the partition
  // is located at the first sector instead MBR ("superfloppy format")
  // see also http://mirror.href.com/thestarman/asm/mbr/PartTypes.htm
  if( ptype != 0x04 && ptype != 0x06 && ptype != 0x0b && ptype != 0x0c && ptype != 0x0e ) {
    pstart = 0;
    DEBUG_MSG("Partition 0 start sector %u (invalid type, assuming superfloppy format)\n", pstart);
  } else {
    DEBUG_MSG("Partition 0 start sector %u active 0x%02x type 0x%02x size %u\n", pstart, pactive, ptype, psize);
  }
    
  if( DFS_GetVolInfo(0, sector, pstart, &vi) ) {
    DEBUG_MSG("ERROR: no volume information\n");
    return -1; // error
  }

  DEBUG_MSG("Volume label '%s'\n", vi.label);
  DEBUG_MSG("%u sector/s per cluster, %u reserved sector/s, volume total %u sectors.\n", vi.secperclus, vi.reservedsecs, vi.numsecs);
  DEBUG_MSG("%u sectors per FAT, first FAT at sector #%u, root dir at #%u.\n",vi.secperfat,vi.fat1,vi.rootdir);
  DEBUG_MSG("(For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n");
  DEBUG_MSG("%u root dir entries, data area commences at sector #%u.\n",vi.rootentries,vi.dataarea);
  char file_system[20];
  if( vi.filesystem == FAT12 )
    strcpy(file_system, "FAT12");
  else if (vi.filesystem == FAT16)
    strcpy(file_system, "FAT16");
  else if (vi.filesystem == FAT32)
    strcpy(file_system, "FAT32");
  else
    strcpy(file_system, "unknown FS");
  DEBUG_MSG("%u clusters (%u bytes) in data area, filesystem IDd as %s\n", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE, file_system);
    
  if( vi.filesystem != FAT12 && vi.filesystem != FAT16 && vi.filesystem != FAT32 ) {
    DEBUG_MSG("ERROR: unknown file system!\n");
    return -1; // error
  }

  di.scratch = sector;
  if( DFS_OpenDir(&vi, "", &di) ) {
    DEBUG_MSG("ERROR: opening root directory - try mounting the partition again\n");
    return -1; // error
  }

  DEBUG_MSG("Content of root directory:\n");
  u32 num_files = 0;
  while( !DFS_GetNext(&vi, &di, &de) ) {
    if( de.name[0] ) {
      u8 file_name[13];
      ++num_files;
      DFS_DirToCanonical(file_name, de.name);
      DEBUG_MSG("  %s\n", file_name);
    }
  }
  DEBUG_MSG("Found %u files.\n", num_files);
  DEBUG_MSG("--------------------\n");

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
  static u8 sdcard_available = 0;

  // init LCD
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("see README.txt   ");
  MIOS32_LCD_CursorSet(0, 1);
  MIOS32_LCD_PrintString("for details     ");

  // turn off LED (will be turned on once SD card connected/detected)
  MIOS32_BOARD_LED_Set(0x1, 0x0);

  // endless loop
  while( 1 ) {
    // check if SD card is available
    // High-speed access if SD card was previously available
    u8 prev_sdcard_available = sdcard_available;
    sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

    if( sdcard_available && !prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x1); // turn on LED
#if DEBUG_VERBOSE_LEVEL >= 0
      // always print...
      DEBUG_MSG("SD Card has been connected!\n");
#endif

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("connected       ");

#if DEBUG_VERBOSE_LEVEL >= 1
      if( PrintSDCardInfos() < 0 ) {
	DEBUG_MSG("ERROR while printing SD Card infos!\n");
      } else {
	DEBUG_MSG("SUCCESS: SD Card can be properly read!\n");
      }
#endif
    } else if( !sdcard_available && prev_sdcard_available ) {
      MIOS32_BOARD_LED_Set(0x1, 0x0); // turn off LED
#if DEBUG_VERBOSE_LEVEL >= 0
      // always print...
      DEBUG_MSG("SD Card disconnected!\n");
#endif

      MIOS32_LCD_Clear();
      MIOS32_LCD_CursorSet(0, 0);
      MIOS32_LCD_PrintString("SD Card         ");
      MIOS32_LCD_CursorSet(0, 1);
      MIOS32_LCD_PrintString("disconnected    ");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
//  This hook is called when a complete MIDI event has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedEvent(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a SysEx byte has been received
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedSysEx(mios32_midi_port_t port, u8 sysex_byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a byte has been received via COM interface
/////////////////////////////////////////////////////////////////////////////
void APP_NotifyReceivedCOM(mios32_com_port_t port, u8 byte)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}
