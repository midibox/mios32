// $Id: mid_file.c 771 2009-11-10 21:10:43Z tk $
/*
 * File Access Routines
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
#include <string.h>
#include <uip.h>

#include "httpd-fs.h"
#include "fs.h"
#include "app.h"

#include <dosfs.h>


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 2
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define FS_FILES_PATH "htdocs"



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the file position and length
static u32 file_pos;
static u32 file_len;
static u8  ui_file_name[13];

// DOS FS variables
static u8 sector[SECTOR_SIZE];
static VOLINFO vi;
static DIRINFO di;
static DIRENT de;
static FILEINFO fi;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 FS_Init(u32 mode)
{
  // initial midifile name and size
  strcpy(ui_file_name, "waiting...");
  file_len = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the filename of the .mid file for the user interface
// Contains an error message if file wasn't loaded correctly
/////////////////////////////////////////////////////////////////////////////
char *FS_UI_NameGet(void)
{
  return ui_file_name;
}


/////////////////////////////////////////////////////////////////////////////
// Mount the file system
// Should be called from the task which periodically checks the availability
// of SD Card (-> app.c, search for MIOS32_SDCARD_CheckAvailable)
/////////////////////////////////////////////////////////////////////////////
s32 FS_mount_fs(void)
{
  u32 pstart, psize;
  u8  pactive, ptype;

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
    DEBUG_MSG("- CSDStruct: %u\n", csd.CSDStruct);
    DEBUG_MSG("- SysSpecVersion: %u\n", csd.SysSpecVersion);
    DEBUG_MSG("- Reserved1: %u\n", csd.Reserved1);
    DEBUG_MSG("- TAAC: %u\n", csd.TAAC);
    DEBUG_MSG("- NSAC: %u\n", csd.NSAC);
    DEBUG_MSG("- MaxBusClkFrec: %u\n", csd.MaxBusClkFrec);
    DEBUG_MSG("- CardComdClasses: %u\n", csd.CardComdClasses);
    DEBUG_MSG("- RdBlockLen: %u\n", csd.RdBlockLen);
    DEBUG_MSG("- PartBlockRead: %u\n", csd.PartBlockRead);
    DEBUG_MSG("- WrBlockMisalign: %u\n", csd.WrBlockMisalign);
    DEBUG_MSG("- RdBlockMisalign: %u\n", csd.RdBlockMisalign);
    DEBUG_MSG("- DSRImpl: %u\n", csd.DSRImpl);
    DEBUG_MSG("- Reserved2: %u\n", csd.Reserved2);
    DEBUG_MSG("- DeviceSize: %u\n", csd.DeviceSize);
    DEBUG_MSG("- MaxRdCurrentVDDMin: %u\n", csd.MaxRdCurrentVDDMin);
    DEBUG_MSG("- MaxRdCurrentVDDMax: %u\n", csd.MaxRdCurrentVDDMax);
    DEBUG_MSG("- MaxWrCurrentVDDMin: %u\n", csd.MaxWrCurrentVDDMin);
    DEBUG_MSG("- MaxWrCurrentVDDMax: %u\n", csd.MaxWrCurrentVDDMax);
    DEBUG_MSG("- DeviceSizeMul: %u\n", csd.DeviceSizeMul);
    DEBUG_MSG("- EraseGrSize: %u\n", csd.EraseGrSize);
    DEBUG_MSG("- EraseGrMul: %u\n", csd.EraseGrMul);
    DEBUG_MSG("- WrProtectGrSize: %u\n", csd.WrProtectGrSize);
    DEBUG_MSG("- WrProtectGrEnable: %u\n", csd.WrProtectGrEnable);
    DEBUG_MSG("- ManDeflECC: %u\n", csd.ManDeflECC);
    DEBUG_MSG("- WrSpeedFact: %u\n", csd.WrSpeedFact);
    DEBUG_MSG("- MaxWrBlockLen: %u\n", csd.MaxWrBlockLen);
    DEBUG_MSG("- WriteBlockPaPartial: %u\n", csd.WriteBlockPaPartial);
    DEBUG_MSG("- Reserved3: %u\n", csd.Reserved3);
    DEBUG_MSG("- ContentProtectAppli: %u\n", csd.ContentProtectAppli);
    DEBUG_MSG("- FileFormatGrouop: %u\n", csd.FileFormatGrouop);
    DEBUG_MSG("- CopyFlag: %u\n", csd.CopyFlag);
    DEBUG_MSG("- PermWrProtect: %u\n", csd.PermWrProtect);
    DEBUG_MSG("- TempWrProtect: %u\n", csd.TempWrProtect);
    DEBUG_MSG("- FileFormat: %u\n", csd.FileFormat);
    DEBUG_MSG("- ECC: %u\n", csd.ECC);
    DEBUG_MSG("- msd_CRC: 0x%02x\n", csd.msd_CRC);
    DEBUG_MSG("- Reserved4: %u\n", csd.Reserved4);
    DEBUG_MSG("--------------------\n");
  }
  
  
  // Obtain pointer to first partition on first (only) unit
  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);

  if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FS] Cannot find first partition!\n");
#endif
    return -1;
  }

  // check for partition type, if we don't get one of these types, it can be assumed that the partition
  // is located at the first sector instead MBR ("superfloppy format")
  // see also http://mirror.href.com/thestarman/asm/mbr/PartTypes.htm
  if( ptype != 0x04 && ptype != 0x06 && ptype != 0x0b && ptype != 0x0c && ptype != 0x0e ) {
    pstart = 0;
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FS] Partition 0 start sector %u (invalid type 0x%02x, assuming superfloppy format)\n", pstart,ptype);
#endif
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FS] Partition 0 start sector %u active 0x%02x type 0x%02x size %u\n", pstart, pactive, ptype, psize);
#endif
  }

  if (DFS_GetVolInfo(0, sector, pstart, &vi)) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FS] Error getting volume information\n");
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FS] Volume label '%s'\n", vi.label);
  DEBUG_MSG("[FS] %u sector/s per cluster, %u reserved sector/s, volume total %u sectors.\n", vi.secperclus, vi.reservedsecs, vi.numsecs);
  DEBUG_MSG("[FS] %u sectors per FAT, first FAT at sector #%u, root dir at #%u.\n",vi.secperfat,vi.fat1,vi.rootdir);
  DEBUG_MSG("[FS] (For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n");
  DEBUG_MSG("[FS] %u root dir entries, data area commences at sector #%u.\n",vi.rootentries,vi.dataarea);
  char file_system[20];
  if( vi.filesystem == FAT12 )
    strcpy(file_system, "FAT12");
  else if (vi.filesystem == FAT16)
    strcpy(file_system, "FAT16");
  else if (vi.filesystem == FAT32)
    strcpy(file_system, "FAT32");
  else
    strcpy(file_system, "unknown FS");
  DEBUG_MSG("[FS] %u clusters (%u bytes) in data area, filesystem IDd as %s\n", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE, file_system);
#endif

  if( vi.filesystem != FAT12 && vi.filesystem != FAT16 && vi.filesystem != FAT32 )
    return -1; // unknown file system

  // enable caching
  // attention:  this feature has to be explicitely enabled, as it isn't reentrant
  // and requires to use the same buffer pointer whenever reading a file.
  DFS_CachingEnabledSet(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Open a file with given filename
/////////////////////////////////////////////////////////////////////////////
s32 FS_open(char *filename, struct httpd_fs_file *file)
{
  MUTEX_SDCARD_TAKE
  strncpy(ui_file_name, filename, 13);
 
  char filepath[MAX_PATH];
  strncpy(filepath, FS_FILES_PATH, MAX_PATH);
  strncat(filepath, filename, MAX_PATH);
  if( DFS_OpenFile(&vi, filepath, DFS_READ, sector, &file->fi)) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FS] error opening file '%s'!\n", filepath);
#endif
    MUTEX_SDCARD_GIVE
    file->pos = 0;
    file->len = 0;
    return -1;
  }
  MUTEX_SDCARD_GIVE

  // got it
  file->pos = 0;
  file->len=file->fi.filelen;
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[FS] opened '%s' of length %u\n", filepath, file->len);
#endif
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads <file->len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 FS_read(void *buffer, struct httpd_fs_file *file)
{
  u32 successcount;

  MUTEX_SDCARD_TAKE
  DFS_ReadFile(&file->fi, sector, buffer, &successcount, (file->len));
  MUTEX_SDCARD_GIVE

  if( successcount != (file->len)) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FS] unexpected read error - DFS cannot access offset %u (len: %u, got: %u)\n", file->pos,file->len, successcount);
#endif
  } 
  
#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("[FS] DFS Reading Offset %u (len: %u, got: %u)\n", file->pos, file->len, successcount);
#endif

  file->pos += successcount; // no error, just failsafe: hold the reference file position, regardless of successcount

  return successcount;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 FS_eof(void)
{
  if( file_pos >= file_len )
    return 1; // end of file reached

  return 0;
}

s32 FS_close(void)
{
  file_pos=0;
  file_len=0;
  MUTEX_SDCARD_GIVE
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 FS_seek(u32 pos)
{
  file_pos = pos;
  if( file_pos >= file_len )
    return -1; // end of file reached
  MUTEX_SDCARD_TAKE
  DFS_Seek(&fi, pos, sector);
  MUTEX_SDCARD_GIVE

  return 0;
}
