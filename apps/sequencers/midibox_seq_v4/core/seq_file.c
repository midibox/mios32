// $Id$
/*
 * File access functions
 *
 * Frontend functions to read/write files.
 * Optimized for Memory Size and Speed!
 *
 * For the whole application only single file handlers for read and write
 * operations are available. They are shared globally to save memory (because
 * each FatFs handler allocates more than 512 bytes to store the last read
 * sector)
 *
 * For read operations it is possible to re-open a file via a seq_file_t reference
 * so that no directory access is required to find the first sector of the
 * file (again).
 *
 * NOTE: before accessing the SD Card, the upper level function should
 * synchronize with the SD Card semaphore!
 *   MUTEX_SDCARD_TAKE; // to take the semaphore
 *   MUTEX_SDCARD_GIVE; // to release the semaphore
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

#include <ff.h>
#include <string.h>

#include "tasks.h"

#include "seq_ui.h"
#include "seq_core.h"

#include "seq_file.h"

#include "seq_file_b.h"
#include "seq_file_m.h"
#include "seq_file_s.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_t.h"
#include "seq_file_hw.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are backup directories located?
// recommented: "backup/"
// backup subdirs (backup/1, backup/2, backup/3, ..." have to be manually
// created, since DosFS doesn't support the creation of new dirs
#define SEQ_FILE_BACKUP_PATH "/BACKUP"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// last error status returned by DFS
// can be used as additional debugging help if SEQ_FILE_*ERR returned by function
u32 seq_file_dfs_errno;

// contains the backup directory during a backup
// used by SEQ_UI to print a message on screen during a backup is created
char *seq_file_backup_notification;

// for percentage display
u8 seq_file_copy_percentage;
u8 seq_file_backup_percentage;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_FILE_MountFS(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// FatFs variables
#define SECTOR_SIZE _MAX_SS

// Work area (file system object) for logical drives
static FATFS fs;

// complete file structure for read/write accesses
static FIL seq_file_read;
static u8 seq_file_read_is_open; // only for safety purposes
static FIL seq_file_write;
static u8 seq_file_write_is_open; // only for safety purposes

// SD Card status
static u8 sdcard_available;
static u8 volume_available;
static u32 volume_free_bytes;

// buffer for copy operations and SysEx sender
#define TMP_BUFFER_SIZE _MAX_SS
static u8 tmp_buffer[TMP_BUFFER_SIZE];

static u8 status_msg_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Init(u32 mode)
{
  seq_file_read_is_open = 0;
  seq_file_write_is_open = 0;
  sdcard_available = 0;
  volume_available = 0;
  volume_free_bytes = 0;

  seq_file_backup_notification = NULL;
  seq_file_copy_percentage = 0;
  seq_file_backup_percentage = 0;

  // init SDCard access
  s32 error = MIOS32_SDCARD_Init(0);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] SD Card interface initialized, status: %d\n", error);
#endif

  // for status message
  status_msg_ctr = 5;

  // init:
  SEQ_FILE_HW_Init(0); // hardware config file access
  SEQ_FILE_C_Init(0); // config file access
  SEQ_FILE_G_Init(0); // groove file access
  SEQ_FILE_B_Init(0); // pattern file access
  SEQ_FILE_M_Init(0); // mixer file access
  SEQ_FILE_S_Init(0); // song file access
  SEQ_FILE_T_Init(0); // track preset file access


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called periodically to check the availability
// of the SD Card
//
// Once the chard has been detected, all banks will be read
// returns < 0 on errors (error codes are documented in seq_file.h)
// returns 1 if SD card has been connected
// returns 2 if SD card has been disconnected
// returns 3 if status message should be print
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_CheckSDCard(void)
{
  // check if SD card is available
  // High-speed access if SD card was previously available
  u8 prev_sdcard_available = sdcard_available;
  sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

  if( sdcard_available && !prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] SD Card has been connected!\n");
#endif

    s32 error = SEQ_FILE_MountFS();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] Tried to mount file system, status: %d\n", error);
#endif

    if( error < 0 ) {
      // ensure that volume flagged as not available
      volume_available = 0;

      return error; // break here!
    }

    // load all file infos
    SEQ_FILE_B_LoadAllBanks();
    SEQ_FILE_M_LoadAllBanks();
    SEQ_FILE_S_LoadAllBanks();
    SEQ_FILE_G_Load();
    SEQ_FILE_C_Load();
    SEQ_FILE_HW_Load();

    // status message after 3 seconds
    status_msg_ctr = 3;

    return 1; // SD card has been connected

  } else if( !sdcard_available && prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] SD Card disconnected!\n");
#endif
    volume_available = 0;

    // invalidate all file infos
    SEQ_FILE_B_UnloadAllBanks();
    SEQ_FILE_M_UnloadAllBanks();
    SEQ_FILE_S_UnloadAllBanks();
    SEQ_FILE_G_Unload();
    SEQ_FILE_C_Unload();
    SEQ_FILE_HW_Unload();

    return 2; // SD card has been disconnected
  }

  if( status_msg_ctr ) {
    if( !--status_msg_ctr )
      return 3;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Mount the file system
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_MountFS(void)
{
  FRESULT res;

  seq_file_read_is_open = 0;
  seq_file_write_is_open = 0;

  if( (res=f_mount(0, &fs)) != FR_OK ) {
    DEBUG_MSG("[SEQ_FILE] Failed to mount SD Card - error status: %d\n", res);
    return -1; // error
  }

  char path[10];
  strcpy(path, "/");
  static DIR dir;
  if( (res=f_opendir(&dir, path)) != FR_OK ) {
    DEBUG_MSG("[SEQ_FILE] Failed to open root directory - error status: %d\n", res);
    return -2; // error
  }

  volume_available = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the number of free bytes by scanning the FAT for
// unused clusters.
// should be called before SEQ_FILE_Volume* bytes are read
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_UpdateFreeBytes(void)
{
  volume_free_bytes = 0xffffffff;

#if 0
  // takes very long! (e.g. my SD card has ca. 2000 clusters, 
  // it takes ca. 3 seconds to determine free clusters)
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] UpdateFreeBytes stopped - no volume!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  u32 scratchcache = 0;
  u32 i;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] Starting UpdateFreeBytes, scan for %u clusters\n", vi.numclusters);
#endif

  for(i=2; i<vi.numclusters; ++i) {
    if( !DFS_GetFAT(&vi, sector, &scratchcache, i) )
      volume_free_bytes += vi.secperclus * SECTOR_SIZE;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] Finished UpdateFreeBytes: %u\n", volume_free_bytes);
#endif
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if SD card available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_SDCardAvailable(void)
{
  return sdcard_available;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if volume available
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_VolumeAvailable(void)
{
  return volume_available;
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_FILE_VolumeBytesFree(void)
{
  // TODO
  return volume_free_bytes;
}


/////////////////////////////////////////////////////////////////////////////
// returns total number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_FILE_VolumeBytesTotal(void)
{
#if 0
  return vi.numclusters * vi.secperclus * SECTOR_SIZE;
#else
  // TODO
  return 1;
#endif
}


/////////////////////////////////////////////////////////////////////////////
// returns volume label
/////////////////////////////////////////////////////////////////////////////
char *SEQ_FILE_VolumeLabel(void)
{
#if 0
  return vi.label;
#else
  // TODO
  return "TODO";
#endif
}


/////////////////////////////////////////////////////////////////////////////
// opens a file for reading
// Note: to save memory, one a single file is allowed to be opened per
// time - always use SEQ_FILE_ReadClose() before opening a new file!
// Use SEQ_FILE_ReadReOpen() to continue reading
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadOpen(seq_file_t* file, char *filepath)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] Opening file '%s'\n", filepath);
#endif

  if( seq_file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE] FAILURE: tried to open file '%s' for reading, but previous file hasn't been closed!\n", filepath);
#endif
    return SEQ_FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  if( (seq_file_dfs_errno=f_open(&seq_file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] Error opening file - try mounting the partition again\n");
#endif

    s32 error;
    if( (error = SEQ_FILE_MountFS()) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] mounting failed with status: %d\n", error);
#endif
      return SEQ_FILE_ERR_SD_CARD;
    }

    if( (seq_file_dfs_errno=f_open(&seq_file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] Still not able to open file - giving up!\n");
#endif
      return SEQ_FILE_ERR_OPEN_READ;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] found '%s' of length %u\n", filepath, seq_file_read.fsize);
#endif

  // file is opened
  seq_file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// reopens a file for reading
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadReOpen(seq_file_t* file)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] Reopening file\n");
#endif

  if( seq_file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE] FAILURE: tried to reopen file, but previous file hasn't been closed!\n");
#endif
    return SEQ_FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  // restore file variables from seq_file_t
  seq_file_read.flag = file->flag;
  seq_file_read.csect = file->csect;
  seq_file_read.fptr = file->fptr;
  seq_file_read.fsize = file->fsize;
  seq_file_read.org_clust = file->org_clust;
  seq_file_read.curr_clust = file->curr_clust;
  seq_file_read.dsect = file->dsect;
  seq_file_read.dir_sect = file->dir_sect;
  seq_file_read.dir_ptr = file->dir_ptr;

  // file is opened (again)
  seq_file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Closes a file which has been read
// File can be re-opened if required thereafter w/o performance issues
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadClose(seq_file_t *file)
{
  // store current file variables in seq_file_t
  file->flag = seq_file_read.flag;
  file->csect = seq_file_read.csect;
  file->fptr = seq_file_read.fptr;
  file->fsize = seq_file_read.fsize;
  file->org_clust = seq_file_read.org_clust;
  file->curr_clust = seq_file_read.curr_clust;
  file->dsect = seq_file_read.dsect;
  file->dir_sect = seq_file_read.dir_sect;
  file->dir_ptr = seq_file_read.dir_ptr;

  // file has been closed
  seq_file_read_is_open = 0;


  // don't close file via f_close()! We allow to open the file again
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadSeek(u32 offset)
{
  return (f_lseek(&seq_file_read, offset) == FR_OK) ? 0 : SEQ_FILE_ERR_SEEK;
}


/////////////////////////////////////////////////////////////////////////////
// Read from file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadBuffer(u8 *buffer, u32 len)
{
  UINT successcount;

  // exit if volume not available
  if( !volume_available )
    return SEQ_FILE_ERR_NO_VOLUME;

  if( (seq_file_dfs_errno=f_read(&seq_file_read, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n", seq_file_read.fptr, seq_file_dfs_errno);
#endif
      return SEQ_FILE_ERR_READ;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Wrong successcount while reading from position 0x%08x (count: %d)\n", seq_file_read.fptr, successcount);
#endif
    return SEQ_FILE_ERR_READCOUNT;
  }

  return 0; // no error
}

s32 SEQ_FILE_ReadLine(u8 *buffer, u32 max_len)
{
  s32 status;
  u32 num_read = 0;

  while( seq_file_read.fptr < seq_file_read.fsize ) {
    status = SEQ_FILE_ReadBuffer(buffer, 1);

    if( status < 0 )
      return status;

    ++num_read;

    if( *buffer == '\n' || *buffer == '\r' )
      break;

    if( num_read < max_len )
      ++buffer;
  }

  // replace newline by terminator
  *buffer = 0;

  return num_read;
}

s32 SEQ_FILE_ReadByte(u8 *byte)
{
  SEQ_FILE_ReadBuffer(byte, 1);
}

s32 SEQ_FILE_ReadHWord(u16 *hword)
{
  s32 status = 0;
  // ensure little endian coding, therefore 2 reads
  *hword = 0x0000;
  u8 tmp;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *hword |= (u16)tmp << 0;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *hword |= (u16)tmp << 8;
  return status;
}

s32 SEQ_FILE_ReadWord(u32 *word)
{
  s32 status = 0;
  // ensure little endian coding, therefore 4 reads
  *word = 0x00000000;
  u8 tmp;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *word |= (u16)tmp << 0;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *word |= (u16)tmp << 8;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *word |= (u16)tmp << 16;
  status |= SEQ_FILE_ReadBuffer(&tmp, 1);
  *word |= (u16)tmp << 24;
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file for writing
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteOpen(char *filepath, u8 create)
{
  if( seq_file_write_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE] FAILURE: tried to open file '%s' for writing, but previous file hasn't been closed!\n", filepath);
#endif
    return SEQ_FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE;
  }

  if( (seq_file_dfs_errno=f_open(&seq_file_write, filepath, (create ? FA_CREATE_ALWAYS : FA_OPEN_EXISTING) | FA_WRITE)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] error opening file '%s' for writing!\n", filepath);
#endif
    return SEQ_FILE_ERR_OPEN_WRITE;
  }

  // remember state
  seq_file_write_is_open = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Closes a file by writing the last bytes
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteClose(void)
{
  s32 status = 0;
  UINT successcount;

  // close file
  f_close(&seq_file_write);

  seq_file_write_is_open = 0;

  return status;
}



/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteSeek(u32 offset)
{
  return (f_lseek(&seq_file_write, offset) == FR_OK) ? 0 : SEQ_FILE_ERR_SEEK;
}


/////////////////////////////////////////////////////////////////////////////
// Returns current size of write file
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_FILE_WriteGetCurrentSize(void)
{
  if( !seq_file_write_is_open )
    return 0;
  return seq_file_write.fsize;
}


/////////////////////////////////////////////////////////////////////////////
// Writes into a file with caching mechanism (actual write at end of sector)
// File has to be closed via SEQ_FILE_WriteClose() after the last byte has
// been written
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteBuffer(u8 *buffer, u32 len)
{
  // exit if volume not available
  if( !volume_available )
    return SEQ_FILE_ERR_NO_VOLUME;

  UINT successcount;
  if( (seq_file_dfs_errno=f_write(&seq_file_write, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Failed to write buffer, status: %u\n", seq_file_dfs_errno);
#endif
    return SEQ_FILE_ERR_WRITE;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Wrong successcount while writing buffer (count: %d)\n", successcount);
#endif
    return SEQ_FILE_ERR_WRITECOUNT;
  }

  return 0; // no error
}

s32 SEQ_FILE_WriteByte(u8 byte)
{
  return SEQ_FILE_WriteBuffer(&byte, 1);
}

s32 SEQ_FILE_WriteHWord(u16 hword)
{
  s32 status = 0;
  // ensure little endian coding, therefore 2 writes
  u8 tmp;
  tmp = (u8)(hword >> 0);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  tmp = (u8)(hword >> 8);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  return status;
}

s32 SEQ_FILE_WriteWord(u32 word)
{
  s32 status = 0;
  // ensure little endian coding, therefore 4 writes
  u8 tmp;
  tmp = (u8)(word >> 0);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  tmp = (u8)(word >> 8);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  tmp = (u8)(word >> 16);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  tmp = (u8)(word >> 24);
  status |= SEQ_FILE_WriteBuffer(&tmp, 1);
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if file exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_FileExists(char *filepath)
{
  if( f_open(&seq_file_read, filepath, FA_OPEN_EXISTING | FA_READ) != FR_OK )
    return 0; // file doesn't exist
  f_close(&seq_file_read);
  return 1; // file exists
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if directory exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_DirExists(char *path)
{
  DIR dir;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_DirExists] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  return (seq_file_dfs_errno=f_opendir(&dir, path)) == FR_OK;
}


/////////////////////////////////////////////////////////////////////////////
// This function prints some useful SD card informations on the MIOS terminal
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PrintSDCardInfos(void)
{
  int status = 0;
  FRESULT res;
  DIR dir;
  FILINFO fileinfo;

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
  
  // note: we have to open a directory to update the disk informations (not done by f_mount)
  char path[100];
  strcpy(path, "/");
  if( (res=f_opendir(&dir, path)) != FR_OK ) {
    DEBUG_MSG("Failed to open root directory - error status: %d\n", res);
  } else {
    DEBUG_MSG("Content of root directory:\n");
    for (;;) {
      res = f_readdir(&dir, &fileinfo);
      if (res != FR_OK || fileinfo.fname[0] == 0) break;
      if (fileinfo.fname[0] == '.') continue;
      char *fn;
#if _USE_LFN
      fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else
      fn = fileinfo.fname;
#endif

      if (fileinfo.fattrib & AM_DIR) {
	if( path[0] == '/' && path[1] == 0 )
	  DEBUG_MSG("%s/\n", fn);
	else
	  DEBUG_MSG("%s/\n", path, fn);
      } else {
	if( path[0] == '/' && path[1] == 0 )
	  DEBUG_MSG("%s\n", fn);
	else
	  DEBUG_MSG("%s\n", path, fn);
      }
    }
  }


  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// returns 1 if it is required to format any file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_FormattingRequired(void)
{
  u8 bank;
  for(bank=0; bank<8; ++bank)
    if( !SEQ_FILE_B_NumPatterns(bank) )
      return 1;

  if( !SEQ_FILE_M_NumMaps() )
    return 1;

  if( !SEQ_FILE_S_NumSongs() )
    return 1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function formats Bank/Mixer/Song files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Format(void)
{
  s32 status = 0;
  u8 num_operations = 8 + 1 + 1;
  char filename_buffer[20];
  seq_file_backup_notification = filename_buffer;

  seq_file_copy_percentage = 0; // for percentage display

  // create non-existing banks if required
  u8 bank;
  for(bank=0; bank<8; ++bank) {
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)bank) / num_operations);

    if( !SEQ_FILE_B_NumPatterns(bank) ) {
      // create bank
      sprintf(seq_file_backup_notification, "MBSEQ_B%d.V4", bank+1);

      if( (status=SEQ_FILE_B_Create(bank)) < 0 )
	return status;

      // fill patterns with useful data
      int pattern;
      int num_patterns = SEQ_FILE_B_NumPatterns(bank);
      for(pattern=0; pattern<num_patterns; ++pattern) {
	seq_file_copy_percentage = (u8)(((u32)100 * (u32)pattern) / num_patterns); // for percentage display
	u8 group = bank % SEQ_CORE_NUM_GROUPS; // note: bank selects source group

	if( (status=SEQ_FILE_B_PatternWrite(bank, pattern, group)) < 0 )
	  return status;
      }

      // open bank
      if( (status=SEQ_FILE_B_Open(bank)) < 0 )
	return status;
    }
  }


  // create non-existing mixer maps if required
  if( !SEQ_FILE_M_NumMaps() ) {
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)8) / num_operations);

    // create maps
    sprintf(seq_file_backup_notification, "MBSEQ_M.V4");

    if( (status=SEQ_FILE_M_Create()) >= 0 ) {
      int map;
      int num_maps = SEQ_FILE_M_NumMaps();
      for(map=0; map<num_maps; ++map) {
	seq_file_copy_percentage = (u8)(((u32)100 * (u32)map) / num_maps); // for percentage display
	if( (status = SEQ_FILE_M_MapWrite(map)) < 0 )
	  return status;
      }

      if( (status=SEQ_FILE_M_Open()) < 0 )
	return status;
    }
  }

  // create non-existing song slots if required
  if( !SEQ_FILE_S_NumSongs() ) {
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)9) / num_operations);

    sprintf(seq_file_backup_notification, "MBSEQ_S.V4");

    // create songs
    if( (status=SEQ_FILE_S_Create()) >= 0 ) {
      int song;
      int num_songs = SEQ_FILE_S_NumSongs();
      for(song=0; song<num_songs; ++song) {
	seq_file_copy_percentage = (u8)(((u32)100 * (u32)song) / num_songs); // for percentage display
	if( (status = SEQ_FILE_S_SongWrite(song)) )
	  return status;
      }

      if( (status=SEQ_FILE_S_Open()) < 0 )
	return status;
    }
  }
      
  // no need to check for existing config file (will be created once config data is stored)
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function copies a file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Copy(char *src_file, char *dst_file, u8 *tmp_buffer)
{
  s32 status = 0;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_Copy] copy %s to %s\n", src_file, dst_file);
#endif

  if( (seq_file_dfs_errno=f_open(&seq_file_read, src_file, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] %s doesn't exist!\n", src_file);
#endif
    status = SEQ_FILE_ERR_COPY_NO_FILE;
  } else {
    if( (seq_file_dfs_errno=f_open(&seq_file_write, dst_file, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE_Copy] wasn't able to create %s - exit!\n", dst_file);
#endif
      status = SEQ_FILE_ERR_COPY;
      f_close(&seq_file_read);
    }
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] Copy aborted due to previous errors (%d)!\n", status);
#endif
  } else {
    // copy operation
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] Starting copy operation!\n");
#endif

    seq_file_copy_percentage = 0; // for percentage display

    UINT successcount;
    UINT successcount_wr;
    u32 num_bytes = 0;
    do {
      if( (seq_file_dfs_errno=f_read(&seq_file_read, tmp_buffer, TMP_BUFFER_SIZE, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n", seq_file_read.fptr, seq_file_dfs_errno);
#endif
	successcount = 0;
	status = SEQ_FILE_ERR_READ;
      } else if( successcount && f_write(&seq_file_write, tmp_buffer, successcount, &successcount_wr) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n", seq_file_write.fptr, seq_file_dfs_errno);
#endif
	status = SEQ_FILE_ERR_WRITE;
      } else {
	num_bytes += successcount_wr;
	seq_file_copy_percentage = (u8)((100 * num_bytes) / seq_file_read.fsize);
      }
    } while( status == 0 && successcount > 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] Finished copy operation (%d bytes)!\n", num_bytes);
#endif

    f_close(&seq_file_read);
    f_close(&seq_file_write);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function creates a backup of all MBSEQ files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_CreateBackup(void)
{
  // TODO
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_CreateBackup] Content of %s directory:\n", SEQ_FILE_BACKUP_PATH);
#endif

  if( f_opendir(&di, SEQ_FILE_BACKUP_PATH) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: opening %s directory - please create it!\n", SEQ_FILE_BACKUP_PATH);
#endif
    status = SEQ_FILE_ERR_NO_BACKUP_DIR;
  }

  int num_files = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' ) {
      ++num_files;

      char filepath[MAX_PATH];
      sprintf(filepath, "%s/%s/MBSEQ_S.V4", SEQ_FILE_BACKUP_PATH, de.fname);

      if( !SEQ_FILE_FileExists(filepath) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_CreateBackup] %s doesn't exist - taking this dir!\n", filepath);
#endif

	// directory found - start to copy files
#define COPY_FILE_MACRO(name) if( status >= 0 ) { \
	  sprintf(filepath, "%s/%s/%s", SEQ_FILE_BACKUP_PATH, de.fname, name); \
	  seq_file_backup_notification = filepath; \
	  status = SEQ_FILE_Copy(name, filepath, tmp_buffer); \
	  if( status == SEQ_FILE_ERR_COPY_NO_FILE ) status = 0; \
	  ++seq_file_backup_file;				\
	  seq_file_backup_percentage = (u8)(((u32)100 * (u32)seq_file_backup_file) / seq_file_backup_files); \
	}

	// this approach saves some stack - we don't want to allocate more memory by using
	// temporary variables to create src_file and dst_file from an array...
	seq_file_backup_percentage = 0;
	u8 seq_file_backup_files = 13; // for percentage display
	u8 seq_file_backup_file = 0;
	COPY_FILE_MACRO("MBSEQ_HW.V4");
	COPY_FILE_MACRO("MBSEQ_B1.V4");
	COPY_FILE_MACRO("MBSEQ_B2.V4");
	COPY_FILE_MACRO("MBSEQ_B3.V4");
	COPY_FILE_MACRO("MBSEQ_B4.V4");
	COPY_FILE_MACRO("MBSEQ_B5.V4");
	COPY_FILE_MACRO("MBSEQ_B6.V4");
	COPY_FILE_MACRO("MBSEQ_B7.V4");
	COPY_FILE_MACRO("MBSEQ_B8.V4");
	COPY_FILE_MACRO("MBSEQ_G.V4");
	COPY_FILE_MACRO("MBSEQ_M.V4");
	COPY_FILE_MACRO("MBSEQ_C.V4");
	COPY_FILE_MACRO("MBSEQ_S.V4"); // important: should be the last file to notify that backup is complete!

	// stop printing the special message
	seq_file_backup_notification = NULL;

	// stop directory scan
	if( status >= 0 )
	  status = 1; // we were successfull!
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_CreateBackup] %s already exists\n", filepath);
#endif
      }
    }
  }

  if( status != 0 ) // either successful copy operations or errors!
    return status;

  if( !num_files ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] no backup subdir exists - please create some!\n");
#endif
    return SEQ_FILE_ERR_NO_BACKUP_SUBDIR;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_CreateBackup] no free backup subdir - please add some!\n");
#endif
  return SEQ_FILE_ERR_NEED_MORE_BACKUP_SUBDIRS;
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for directories under given path and copies the names
// into a list (e.g. used by seq_ui_sysex.c)
// !!TEMPORARY VERSION!! will be combined with SEQ_FILE_GetFiles later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GetDirs(char *path, char *dir_list, u8 num_of_items, u8 dir_offset)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_GetDirs] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_GetDirs] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = SEQ_FILE_ERR_NO_DIR;
  }

  int num_dirs = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' && (de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {
      ++num_dirs;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif

      if( num_dirs >= dir_offset && num_dirs <= (dir_offset+num_of_items) ) {
	int item_pos = 9 * (num_dirs-1-dir_offset);
	char *p = (char *)&dir_list[item_pos];
	int i;
	for(i=0; i<8; ++i) {
	  char c = de.fname[i];
	  if( !c || c == '.' )
	    break;
	  *p++ = c;
	}
	for(;i<9; ++i)
	  *p++ = ' ';
      }
    }
  }

  return (status < 0) ? status : num_dirs;
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for files under given path and copies the names
// into a list (e.g. used by seq_ui_sysex.c)
// !!TEMPORARY VERSION!! will be combined with SEQ_FILE_GetDirs later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_GetFiles(char *path, char *ext_filter, char *dir_list, u8 num_of_items, u8 dir_offset)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_GetFiles] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_GetFiles] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = SEQ_FILE_ERR_NO_DIR;
  }

  int num_files = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' &&
	!(de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {

      char *p = (char *)&de.fname[0];
      int i;
      for(i=0; i<9; ++i) {
	if( *p == '.' )
	  break;
	else
	  *p++;
      }

      if( *p++ != '.' )
	continue;

      if( strncasecmp(p, ext_filter, 3) != 0 )
	continue;

      ++num_files;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif

      if( num_files >= dir_offset && num_files <= (dir_offset+num_of_items) ) {
	int item_pos = 9 * (num_files-1-dir_offset);
	p = (char *)&dir_list[item_pos];
	for(i=0; i<8; ++i) {
	  char c = de.fname[i];
	  if( !c || c == '.' )
	    break;
	  *p++ = c;
	}
	for(;i<9; ++i)
	  *p++ = ' ';
      }
    }
  }

  return (status < 0) ? status : num_files;
}


/////////////////////////////////////////////////////////////////////////////
// This function sends a .syx file from sysex/<dir_name>
// (currently only used by seq_ui_sysex.c, API might change in future)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_SendSyxDump(char *path, mios32_midi_port_t port)
{
  s32 status = 0;
  seq_file_t file;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_SendSyxDump] Open file '%s'\n", path);
#endif

  if( (status=SEQ_FILE_ReadOpen(&file, path)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_SendSyxDump] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // stream SysEx to MIDI port
  UINT successcount;
  u32 num_bytes = 0;
  u8 buffer[10];
  do {
    if( (seq_file_dfs_errno=f_read(&seq_file_read, tmp_buffer, TMP_BUFFER_SIZE, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n", seq_file_read.fptr, seq_file_dfs_errno);
#endif
      successcount = 0;
      status = SEQ_FILE_ERR_SYSEX_READ;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugHexDump(tmp_buffer, successcount);
#endif
      MIOS32_MIDI_SendSysEx(port, tmp_buffer, successcount);
      num_bytes += successcount;
    }
  } while( status == 0 && successcount > 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_SendSyxDump] sent %d bytes!\n", num_bytes);
#endif

  // close file
  status |= SEQ_FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_SendSyxDump] ERROR while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_ERR_SYSEX_READ;
  }

  return num_bytes;
}

