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

#include "seq_mixer.h"
#include "seq_pattern.h"
#include "seq_song.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// name/directory of session
char seq_file_session_name[13];
char seq_file_new_session_name[13];

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

// disk label
static char disk_label[12];

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
  strcpy(seq_file_session_name, "DEFAULT");
  seq_file_new_session_name[0] = 0; // invalidate
  
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

    // load last selected session name
    SEQ_FILE_LoadSessionName();

    // load all file infos
    SEQ_FILE_LoadAllFiles(1); // including HW info

    // status message after 3 seconds
    status_msg_ctr = 3;

    return 1; // SD card has been connected

  } else if( !sdcard_available && prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] SD Card disconnected!\n");
#endif
    volume_available = 0;

    // invalidate all file infos
    SEQ_FILE_UnloadAllFiles();

    // invalidate session
    strcpy(seq_file_session_name, "DEFAULT");
    seq_file_new_session_name[0] = 0; // invalidate

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
  DIR dir;

  seq_file_read_is_open = 0;
  seq_file_write_is_open = 0;

  if( (res=f_mount(0, &fs)) != FR_OK ) {
    DEBUG_MSG("[SEQ_FILE] Failed to mount SD Card - error status: %d\n", res);
    return -1; // error
  }

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[SEQ_FILE] Failed to open root directory - error status: %d\n", res);
    return -2; // error
  }

  // TODO: read from master sector
  disk_label[0] = 0;

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
  FRESULT res;
  DIR dir;
  DWORD free_clust;

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[SEQ_FILE_UpdateFreeBytes] f_opendir failed with status %d!\n", res);
    return SEQ_FILE_ERR_UPDATE_FREE;
  }

  if( (res=f_getfree("/", &free_clust, &dir.fs)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_UpdateFreeBytes] f_getfree failed with status %d!\n", res);
#endif
    return SEQ_FILE_ERR_UPDATE_FREE;
  }

  volume_free_bytes = free_clust * fs.csize * 512;

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
  return volume_free_bytes;
}


/////////////////////////////////////////////////////////////////////////////
// returns total number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 SEQ_FILE_VolumeBytesTotal(void)
{
  return (fs.max_clust-2)*fs.csize * 512;
}


/////////////////////////////////////////////////////////////////////////////
// returns volume label
/////////////////////////////////////////////////////////////////////////////
char *SEQ_FILE_VolumeLabel(void)
{
  return disk_label;
}


/////////////////////////////////////////////////////////////////////////////
// Loads all files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_LoadAllFiles(u8 including_hw)
{
  s32 status = 0;
  status |= SEQ_FILE_B_LoadAllBanks();
  status |= SEQ_FILE_M_LoadAllBanks();
  status |= SEQ_FILE_S_LoadAllBanks();
  status |= SEQ_FILE_G_Load();
  if( including_hw )
    status |= SEQ_FILE_HW_Load();

  if( SEQ_FILE_C_Load() >= 0 ) {
    // change mixer map to the one stored in MBSEQ_C.V4
    SEQ_MIXER_Load(SEQ_MIXER_NumGet());

    // change patterns to the ones stored in MBSEQ_C.V4
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
      SEQ_PATTERN_Change(group, seq_pattern[group]);

    // change song to the one stored in MBSEQ_C.V4
    SEQ_SONG_Load(SEQ_SONG_NumGet());
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// invalidate all file infos
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_UnloadAllFiles(void)
{
  s32 status = 0;
  status |= SEQ_FILE_B_UnloadAllBanks();
  status |= SEQ_FILE_M_UnloadAllBanks();
  status |= SEQ_FILE_S_UnloadAllBanks();
  status |= SEQ_FILE_G_Unload();
  status |= SEQ_FILE_C_Unload();
  status |= SEQ_FILE_HW_Unload();
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores the current name of the session in a file (/SESSIONS/LAST_ONE.V4), so 
// that it can be restored after startup
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_StoreSessionName(void)
{
  s32 status;
  char filepath[30];

  sprintf(filepath, "%s/LAST_ONE.V4", SEQ_FILE_SESSION_PATH);
  status=SEQ_FILE_WriteOpen(filepath, 1);
  if( status >= 0 ) {
    status = SEQ_FILE_WriteBuffer(seq_file_session_name, strlen(seq_file_session_name));
    if( status >= 0 )
      status = SEQ_FILE_WriteByte('\n');
    SEQ_FILE_WriteClose();
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_StoreSessionName] ERROR: failed to store last session name (status: %d)\n", status);
#endif
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Restores the last session from the file /SESSIONS/LAST_ONE.V4
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_LoadSessionName(void)
{
  s32 status;
  char filepath[30];
  seq_file_t file;

  sprintf(filepath, "%s/LAST_ONE.V4", SEQ_FILE_SESSION_PATH);
  if( (status=SEQ_FILE_ReadOpen(&file, filepath)) >= 0 ) {
    char linebuffer[20];
    status = SEQ_FILE_ReadLine((char *)&linebuffer, 20);
    if( status < 0 || strlen(linebuffer) > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_LoadSessionName] ERROR: invalid session name '%s'\n", linebuffer);
#endif
      status = SEQ_FILE_ERR_INVALID_SESSION_NAME;
    } else {
      // take over session name
      strcpy(seq_file_session_name, linebuffer);
    }

    SEQ_FILE_ReadClose(&file);
  }
  return status;
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
  seq_file_read.fs = &fs;
  seq_file_read.id = fs.id;
  seq_file_read.flag = file->flag;
  seq_file_read.csect = file->csect;
  seq_file_read.fptr = file->fptr;
  seq_file_read.fsize = file->fsize;
  seq_file_read.org_clust = file->org_clust;
  seq_file_read.curr_clust = file->curr_clust;
  seq_file_read.dsect = file->dsect;
  seq_file_read.dir_sect = file->dir_sect;
  seq_file_read.dir_ptr = file->dir_ptr;

  // ensure that the right sector is in cache again
  disk_read(seq_file_read.fs->drive, seq_file_read.buf, seq_file_read.dsect, 1);

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
  if( (seq_file_dfs_errno=f_lseek(&seq_file_read, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, seq_file_dfs_errno);
#endif
    return SEQ_FILE_ERR_SEEK;
  }
  return 0; // no error
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
  return SEQ_FILE_ReadBuffer(byte, 1);
}

s32 SEQ_FILE_ReadHWord(u16 *hword)
{
  // ensure little endian coding
  u8 tmp[2];
  s32 status = SEQ_FILE_ReadBuffer(tmp, 2);
  *hword = ((u16)tmp[0] << 0) | ((u16)tmp[1] << 8);
  return status;
}

s32 SEQ_FILE_ReadWord(u32 *word)
{
  // ensure little endian coding
  u8 tmp[4];
  s32 status = SEQ_FILE_ReadBuffer(tmp, 4);
  *word = ((u32)tmp[0] << 0) | ((u32)tmp[1] << 8) | ((u32)tmp[2] << 16) | ((u32)tmp[3] << 24);
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

  // close file
  if( (seq_file_dfs_errno=f_close(&seq_file_write)) != FR_OK )
    status = SEQ_FILE_ERR_WRITECLOSE;

  seq_file_write_is_open = 0;

  return status;
}



/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteSeek(u32 offset)
{
  if( (seq_file_dfs_errno=f_lseek(&seq_file_write, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, seq_file_dfs_errno);
#endif
    return SEQ_FILE_ERR_SEEK;
  }
  return 0; // no error
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
  // ensure little endian coding
  u8 tmp[2];
  tmp[0] = (u8)(hword >> 0);
  tmp[1] = (u8)(hword >> 8);
  return SEQ_FILE_WriteBuffer(tmp, 2);
}

s32 SEQ_FILE_WriteWord(u32 word)
{
  s32 status = 0;
  // ensure little endian coding
  u8 tmp[4];
  tmp[0] = (u8)(word >> 0);
  tmp[1] = (u8)(word >> 8);
  tmp[2] = (u8)(word >> 16);
  tmp[3] = (u8)(word >> 24);
  return SEQ_FILE_WriteBuffer(tmp, 4);
}


/////////////////////////////////////////////////////////////////////////////
// Creates a directory
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_MakeDir(char *path)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_MakeDir] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( (seq_file_dfs_errno=f_mkdir(path)) != FR_OK )
    return SEQ_FILE_ERR_MKDIR;

  return 0; // directory created
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if file exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_FileExists(char *filepath)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_FileExists] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( f_open(&seq_file_read, filepath, FA_OPEN_EXISTING | FA_READ) != FR_OK )
    return 0; // file doesn't exist
  //f_close(&seq_file_read); // never close read files to avoid "invalid object"
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

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// returns 1 if it is required to format any file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_FormattingRequired(void)
{
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank)
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

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( seq_file_new_session_name[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_Format] ERROR: no new session specified!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  // switch to new session name, store old name for the case that we have to switch back
  char prev_session_name[13];
  strcpy(prev_session_name, seq_file_session_name);
  strcpy(seq_file_session_name, seq_file_new_session_name); 


#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_Format] Creating Session %s (previous was %s)\n", seq_file_session_name, prev_session_name);
#endif

  u8 num_operations = SEQ_FILE_B_NUM_BANKS + 1 + 1 + 1 + 1;
  char filename_buffer[30];
  seq_file_backup_notification = filename_buffer;

  seq_file_copy_percentage = 0; // for percentage display

  // create banks
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)bank) / num_operations);
    sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_B%d.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name, bank+1);

    if( (status=SEQ_FILE_B_Create(bank)) < 0 )
      goto SEQ_FILE_Format_failed;

    // fill patterns with useful data
    int pattern;
    int num_patterns = SEQ_FILE_B_NumPatterns(bank);
    for(pattern=0; pattern<num_patterns; ++pattern) {
      seq_file_copy_percentage = (u8)(((u32)100 * (u32)pattern) / num_patterns); // for percentage display
      u8 group = bank % SEQ_CORE_NUM_GROUPS; // note: bank selects source group

      if( (status=SEQ_FILE_B_PatternWrite(bank, pattern, group)) < 0 )
	goto SEQ_FILE_Format_failed;
    }

    // open bank
    if( (status=SEQ_FILE_B_Open(bank)) < 0 )
      goto SEQ_FILE_Format_failed;
  }


  // create mixer maps
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+0)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_M.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  if( (status=SEQ_FILE_M_Create()) >= 0 ) {
    int map;
    int num_maps = SEQ_FILE_M_NumMaps();
    for(map=0; map<num_maps; ++map) {
      seq_file_copy_percentage = (u8)(((u32)100 * (u32)map) / num_maps); // for percentage display
      if( (status = SEQ_FILE_M_MapWrite(map)) < 0 )
	goto SEQ_FILE_Format_failed;
    }

    if( (status=SEQ_FILE_M_Open()) < 0 )
      goto SEQ_FILE_Format_failed;
  }

  // create song
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+1)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_S.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  if( (status=SEQ_FILE_S_Create()) >= 0 ) {
    int song;
    int num_songs = SEQ_FILE_S_NumSongs();
    for(song=0; song<num_songs; ++song) {
      seq_file_copy_percentage = (u8)(((u32)100 * (u32)song) / num_songs); // for percentage display
      if( (status = SEQ_FILE_S_SongWrite(song)) )
	goto SEQ_FILE_Format_failed;
    }

    if( (status=SEQ_FILE_S_Open()) < 0 )
      goto SEQ_FILE_Format_failed;
  }


  // create grooves
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+2)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_G.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  if( (status=SEQ_FILE_G_Write()) < 0 )
    goto SEQ_FILE_Format_failed;


  // create config
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+3)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_C.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  if( (status=SEQ_FILE_C_Write()) < 0 )
    goto SEQ_FILE_Format_failed;


SEQ_FILE_Format_failed:
  if( status >= 0 ) {
    // we were successfull
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] Session %s created successfully!\n", seq_file_session_name);
#endif

    // reload setup
    SEQ_FILE_LoadAllFiles(0); // excluding HW info
  } else {
    // we were not successfull!
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] Session %s failed with status %d!\n", seq_file_session_name, status);
#endif

    // switch back to old session name
    strcpy(seq_file_session_name, prev_session_name); 
  }

  // in any case invalidate new session name
  seq_file_new_session_name[0] = 0;

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
      //f_close(&seq_file_read); // never close read files to avoid "invalid object"
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
	DEBUG_MSG("[SEQ_FILE_Copy] Failed to read sector at position 0x%08x, status: %u\n", seq_file_read.fptr, seq_file_dfs_errno);
#endif
	successcount = 0;
	status = SEQ_FILE_ERR_READ;
      } else if( successcount && f_write(&seq_file_write, tmp_buffer, successcount, &successcount_wr) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_Copy] Failed to write sector at position 0x%08x, status: %u\n", seq_file_write.fptr, seq_file_dfs_errno);
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

    //f_close(&seq_file_read); // never close read files to avoid "invalid object"
    f_close(&seq_file_write);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function creates a backup of all MBSEQ files
// Source: seq_file_session_name
// Destination: seq_file_new_session_name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_CreateBackup(void)
{
  s32 status = 0;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  if( seq_file_new_session_name[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: no new session specified!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  char src_path[40];
  sprintf(src_path, "%s/%s", SEQ_FILE_SESSION_PATH, seq_file_session_name);
  char dst_path[40];
  sprintf(dst_path, "%s/%s", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);

  char src_file[50];
  char dst_file[50];
  // We assume that session directory already has been created in seq_ui_menu.c

#define COPY_FILE_MACRO(name) if( status >= 0 ) { \
    sprintf(src_file, "%s/%s", src_path, name);   \
    sprintf(dst_file, "%s/%s", dst_path, name);   \
    seq_file_backup_notification = dst_file;      \
    status = SEQ_FILE_Copy(src_file, dst_file, tmp_buffer); \
    if( status == SEQ_FILE_ERR_COPY_NO_FILE ) status = 0;   \
    ++seq_file_backup_file;				    \
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)seq_file_backup_file) / seq_file_backup_files); \
  }

  // this approach saves some stack - we don't want to allocate more memory by using
  // temporary variables to create src_file and dst_file from an array...
  seq_file_backup_percentage = 0;
  u8 seq_file_backup_files = SEQ_FILE_B_NUM_BANKS+4; // for percentage display
  u8 seq_file_backup_file = 0;
  COPY_FILE_MACRO("MBSEQ_B1.V4");
  COPY_FILE_MACRO("MBSEQ_B2.V4");
  COPY_FILE_MACRO("MBSEQ_B3.V4");
  COPY_FILE_MACRO("MBSEQ_B4.V4");
  COPY_FILE_MACRO("MBSEQ_G.V4");
  COPY_FILE_MACRO("MBSEQ_M.V4");
  COPY_FILE_MACRO("MBSEQ_C.V4");
  COPY_FILE_MACRO("MBSEQ_S.V4"); // important: should be the last file to notify that backup is complete!

  // stop printing the special message
  seq_file_backup_notification = NULL;

  if( status >= 0 ) {
    // we were successfull!
    status = 1;
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] backup of %s passed, new name: %s\n", seq_file_session_name, seq_file_new_session_name);
#endif

    // take over session name
    strcpy(seq_file_session_name, seq_file_new_session_name); 
  } else if( status < 0 ) { // found errors!
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] backup of %s failed, status %d\n", seq_file_new_session_name, status);
#endif
    status = SEQ_FILE_ERR_COPY;
  }

  // in any case invalidate new session name
  seq_file_new_session_name[0] = 0;

  return status;
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

