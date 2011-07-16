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
 * For read operations it is possible to re-open a file via a synth_file_t reference
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
#include <diskio.h>
#include <string.h>

#include "tasks.h"

#include "midio_file.h"

#include "midio_file_p.h"


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
char midio_file_session_name[13];
char midio_file_new_session_name[13];

// last error status returned by DFS
// can be used as additional debugging help if MIDIO_FILE_*ERR returned by function
u32 midio_file_dfs_errno;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MIDIO_FILE_MountFS(void);


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
static FIL midio_file_read;
static u8 midio_file_read_is_open; // only for safety purposes
static FIL midio_file_write;
static u8 midio_file_write_is_open; // only for safety purposes

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
s32 MIDIO_FILE_Init(u32 mode)
{
  strcpy(midio_file_session_name, "DEFAULT");
  midio_file_new_session_name[0] = 0; // invalidate
  
  midio_file_read_is_open = 0;
  midio_file_write_is_open = 0;
  sdcard_available = 0;
  volume_available = 0;
  volume_free_bytes = 0;

  // init SDCard access
  s32 error = MIOS32_SDCARD_Init(0);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE] SD Card interface initialized, status: %d\n", error);
#endif

  // for status message
  status_msg_ctr = 5;

  // init:
  MIDIO_FILE_P_Init(0); // patch file access


  return error;
}


/////////////////////////////////////////////////////////////////////////////
// This function should be called periodically to check the availability
// of the SD Card
//
// Once the chard has been detected, all banks will be read
// returns < 0 on errors (error codes are documented in midio_file.h)
// returns 1 if SD card has been connected
// returns 2 if SD card has been disconnected
// returns 3 if status message should be print
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_CheckSDCard(void)
{
  // check if SD card is available
  // High-speed access if SD card was previously available
  u8 prev_sdcard_available = sdcard_available;
  sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

  if( sdcard_available && !prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE] SD Card has been connected!\n");
#endif

    s32 error = MIDIO_FILE_MountFS();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE] Tried to mount file system, status: %d\n", error);
#endif

    if( error < 0 ) {
      // ensure that volume flagged as not available
      volume_available = 0;

      return error; // break here!
    }

    // load last selected session name
    MIDIO_FILE_LoadSessionName();

    // load all file infos
    MIDIO_FILE_LoadAllFiles(1); // including HW info

    // status message after 3 seconds
    status_msg_ctr = 3;

    return 1; // SD card has been connected

  } else if( !sdcard_available && prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE] SD Card disconnected!\n");
#endif
    volume_available = 0;

    // invalidate all file infos
    MIDIO_FILE_UnloadAllFiles();

    // invalidate session
    strcpy(midio_file_session_name, "DEFAULT");
    midio_file_new_session_name[0] = 0; // invalidate

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
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 MIDIO_FILE_MountFS(void)
{
  FRESULT res;
  DIR dir;

  midio_file_read_is_open = 0;
  midio_file_write_is_open = 0;

  if( (res=f_mount(0, &fs)) != FR_OK ) {
    DEBUG_MSG("[MIDIO_FILE] Failed to mount SD Card - error status: %d\n", res);
    return -1; // error
  }

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[MIDIO_FILE] Failed to open root directory - error status: %d\n", res);
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
// should be called before MIDIO_FILE_Volume* bytes are read
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_UpdateFreeBytes(void)
{
  FRESULT res;
  DIR dir;
  DWORD free_clust;

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[MIDIO_FILE_UpdateFreeBytes] f_opendir failed with status %d!\n", res);
    return MIDIO_FILE_ERR_UPDATE_FREE;
  }

  if( (res=f_getfree("/", &free_clust, &dir.fs)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_UpdateFreeBytes] f_getfree failed with status %d!\n", res);
#endif
    return MIDIO_FILE_ERR_UPDATE_FREE;
  }

  volume_free_bytes = free_clust * fs.csize * 512;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if SD card available
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_SDCardAvailable(void)
{
  return sdcard_available;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if volume available
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_VolumeAvailable(void)
{
  return volume_available;
}


/////////////////////////////////////////////////////////////////////////////
// returns number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIDIO_FILE_VolumeBytesFree(void)
{
  return volume_free_bytes;
}


/////////////////////////////////////////////////////////////////////////////
// returns total number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIDIO_FILE_VolumeBytesTotal(void)
{
  return (fs.max_clust-2)*fs.csize * 512;
}


/////////////////////////////////////////////////////////////////////////////
// returns volume label
/////////////////////////////////////////////////////////////////////////////
char *MIDIO_FILE_VolumeLabel(void)
{
  return disk_label;
}


/////////////////////////////////////////////////////////////////////////////
// Loads all files
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_LoadAllFiles(u8 including_hw)
{
  s32 status = 0;

  if( including_hw ) {
    //status |= MIDIO_FILE_HW_Load();
  }

  status |= MIDIO_FILE_P_Load();

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// invalidate all file infos
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_UnloadAllFiles(void)
{
  s32 status = 0;
  status |= MIDIO_FILE_P_Unload();
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores the current name of the session in a file (/SESSIONS/LAST_ONE.V4), so 
// that it can be restored after startup
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_StoreSessionName(void)
{
  s32 status;
  char filepath[30];

  sprintf(filepath, "%s/LAST_ONE.V4", MIDIO_FILE_SESSION_PATH);
  status=MIDIO_FILE_WriteOpen(filepath, 1);
  if( status >= 0 ) {
    status = MIDIO_FILE_WriteBuffer((u8 *)midio_file_session_name, strlen(midio_file_session_name));
    if( status >= 0 )
      status = MIDIO_FILE_WriteByte('\n');
    MIDIO_FILE_WriteClose();
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_StoreSessionName] ERROR: failed to store last session name (status: %d)\n", status);
#endif
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Restores the last session from the file /SESSIONS/LAST_ONE.V4
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_LoadSessionName(void)
{
  s32 status;
  char filepath[30];
  midio_file_t file;

  sprintf(filepath, "%s/LAST_ONE.V4", MIDIO_FILE_SESSION_PATH);
  if( (status=MIDIO_FILE_ReadOpen(&file, filepath)) >= 0 ) {
    char linebuffer[20];
    status = MIDIO_FILE_ReadLine((u8 *)&linebuffer, 20);
    if( status < 0 || strlen(linebuffer) > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MIDIO_FILE_LoadSessionName] ERROR: invalid session name '%s'\n", linebuffer);
#endif
      status = MIDIO_FILE_ERR_INVALID_SESSION_NAME;
    } else {
      // take over session name
      strcpy(midio_file_session_name, linebuffer);
    }

    MIDIO_FILE_ReadClose(&file);
  }
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// opens a file for reading
// Note: to save memory, one a single file is allowed to be opened per
// time - always use MIDIO_FILE_ReadClose() before opening a new file!
// Use MIDIO_FILE_ReadReOpen() to continue reading
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_ReadOpen(midio_file_t* file, char *filepath)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE] Opening file '%s'\n", filepath);
#endif

  if( midio_file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE] FAILURE: tried to open file '%s' for reading, but previous file hasn't been closed!\n", filepath);
#endif
    return MIDIO_FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  if( (midio_file_dfs_errno=f_open(&midio_file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE] Error opening file - try mounting the partition again\n");
#endif

    s32 error;
    if( (error = MIDIO_FILE_MountFS()) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[MIDIO_FILE] mounting failed with status: %d\n", error);
#endif
      return MIDIO_FILE_ERR_SD_CARD;
    }

    if( (midio_file_dfs_errno=f_open(&midio_file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[MIDIO_FILE] Still not able to open file - giving up!\n");
#endif
      return MIDIO_FILE_ERR_OPEN_READ;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE] found '%s' of length %u\n", filepath, midio_file_read.fsize);
#endif

  // store current file variables in midio_file_t
  file->flag = midio_file_read.flag;
  file->csect = midio_file_read.csect;
  file->fptr = midio_file_read.fptr;
  file->fsize = midio_file_read.fsize;
  file->org_clust = midio_file_read.org_clust;
  file->curr_clust = midio_file_read.curr_clust;
  file->dsect = midio_file_read.dsect;
  file->dir_sect = midio_file_read.dir_sect;
  file->dir_ptr = midio_file_read.dir_ptr;

  // file is opened
  midio_file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// reopens a file for reading
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_ReadReOpen(midio_file_t* file)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE] Reopening file\n");
#endif

  if( midio_file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE] FAILURE: tried to reopen file, but previous file hasn't been closed!\n");
#endif
    return MIDIO_FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  // restore file variables from midio_file_t
  midio_file_read.fs = &fs;
  midio_file_read.id = fs.id;
  midio_file_read.flag = file->flag;
  midio_file_read.csect = file->csect;
  midio_file_read.fptr = file->fptr;
  midio_file_read.fsize = file->fsize;
  midio_file_read.org_clust = file->org_clust;
  midio_file_read.curr_clust = file->curr_clust;
  midio_file_read.dsect = file->dsect;
  midio_file_read.dir_sect = file->dir_sect;
  midio_file_read.dir_ptr = file->dir_ptr;

  // ensure that the right sector is in cache again
  disk_read(midio_file_read.fs->drive, midio_file_read.buf, midio_file_read.dsect, 1);

  // file is opened (again)
  midio_file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Closes a file which has been read
// File can be re-opened if required thereafter w/o performance issues
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_ReadClose(midio_file_t *file)
{
  // store current file variables in midio_file_t
  file->flag = midio_file_read.flag;
  file->csect = midio_file_read.csect;
  file->fptr = midio_file_read.fptr;
  file->fsize = midio_file_read.fsize;
  file->org_clust = midio_file_read.org_clust;
  file->curr_clust = midio_file_read.curr_clust;
  file->dsect = midio_file_read.dsect;
  file->dir_sect = midio_file_read.dir_sect;
  file->dir_ptr = midio_file_read.dir_ptr;

  // file has been closed
  midio_file_read_is_open = 0;


  // don't close file via f_close()! We allow to open the file again
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_ReadSeek(u32 offset)
{
  if( (midio_file_dfs_errno=f_lseek(&midio_file_read, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, midio_file_dfs_errno);
#endif
    return MIDIO_FILE_ERR_SEEK;
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Read from file
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_ReadBuffer(u8 *buffer, u32 len)
{
  UINT successcount;

  // exit if volume not available
  if( !volume_available )
    return MIDIO_FILE_ERR_NO_VOLUME;

  if( (midio_file_dfs_errno=f_read(&midio_file_read, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[MIDIO_FILE] Failed to read sector at position 0x%08x, status: %u\n", midio_file_read.fptr, midio_file_dfs_errno);
#endif
      return MIDIO_FILE_ERR_READ;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[MIDIO_FILE] Wrong successcount while reading from position 0x%08x (count: %d)\n", midio_file_read.fptr, successcount);
#endif
    return MIDIO_FILE_ERR_READCOUNT;
  }

  return 0; // no error
}

s32 MIDIO_FILE_ReadLine(u8 *buffer, u32 max_len)
{
  s32 status;
  u32 num_read = 0;

  while( midio_file_read.fptr < midio_file_read.fsize ) {
    status = MIDIO_FILE_ReadBuffer(buffer, 1);

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

s32 MIDIO_FILE_ReadByte(u8 *byte)
{
  return MIDIO_FILE_ReadBuffer(byte, 1);
}

s32 MIDIO_FILE_ReadHWord(u16 *hword)
{
  // ensure little endian coding
  u8 tmp[2];
  s32 status = MIDIO_FILE_ReadBuffer(tmp, 2);
  *hword = ((u16)tmp[0] << 0) | ((u16)tmp[1] << 8);
  return status;
}

s32 MIDIO_FILE_ReadWord(u32 *word)
{
  // ensure little endian coding
  u8 tmp[4];
  s32 status = MIDIO_FILE_ReadBuffer(tmp, 4);
  *word = ((u32)tmp[0] << 0) | ((u32)tmp[1] << 8) | ((u32)tmp[2] << 16) | ((u32)tmp[3] << 24);
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file for writing
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_WriteOpen(char *filepath, u8 create)
{
  if( midio_file_write_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE] FAILURE: tried to open file '%s' for writing, but previous file hasn't been closed!\n", filepath);
#endif
    return MIDIO_FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE;
  }

  if( (midio_file_dfs_errno=f_open(&midio_file_write, filepath, (create ? FA_CREATE_ALWAYS : FA_OPEN_EXISTING) | FA_WRITE)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE] error opening file '%s' for writing!\n", filepath);
#endif
    return MIDIO_FILE_ERR_OPEN_WRITE;
  }

  // remember state
  midio_file_write_is_open = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Closes a file by writing the last bytes
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_WriteClose(void)
{
  s32 status = 0;

  // close file
  if( (midio_file_dfs_errno=f_close(&midio_file_write)) != FR_OK )
    status = MIDIO_FILE_ERR_WRITECLOSE;

  midio_file_write_is_open = 0;

  return status;
}



/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_WriteSeek(u32 offset)
{
  if( (midio_file_dfs_errno=f_lseek(&midio_file_write, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, midio_file_dfs_errno);
#endif
    return MIDIO_FILE_ERR_SEEK;
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns current size of write file
/////////////////////////////////////////////////////////////////////////////
u32 MIDIO_FILE_WriteGetCurrentSize(void)
{
  if( !midio_file_write_is_open )
    return 0;
  return midio_file_write.fsize;
}


/////////////////////////////////////////////////////////////////////////////
// Writes into a file with caching mechanism (actual write at end of sector)
// File has to be closed via MIDIO_FILE_WriteClose() after the last byte has
// been written
// returns < 0 on errors (error codes are documented in midio_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_WriteBuffer(u8 *buffer, u32 len)
{
  // exit if volume not available
  if( !volume_available )
    return MIDIO_FILE_ERR_NO_VOLUME;

  UINT successcount;
  if( (midio_file_dfs_errno=f_write(&midio_file_write, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[MIDIO_FILE] Failed to write buffer, status: %u\n", midio_file_dfs_errno);
#endif
    return MIDIO_FILE_ERR_WRITE;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[MIDIO_FILE] Wrong successcount while writing buffer (count: %d)\n", successcount);
#endif
    return MIDIO_FILE_ERR_WRITECOUNT;
  }

  return 0; // no error
}

s32 MIDIO_FILE_WriteByte(u8 byte)
{
  return MIDIO_FILE_WriteBuffer(&byte, 1);
}

s32 MIDIO_FILE_WriteHWord(u16 hword)
{
  // ensure little endian coding
  u8 tmp[2];
  tmp[0] = (u8)(hword >> 0);
  tmp[1] = (u8)(hword >> 8);
  return MIDIO_FILE_WriteBuffer(tmp, 2);
}

s32 MIDIO_FILE_WriteWord(u32 word)
{
  // ensure little endian coding
  u8 tmp[4];
  tmp[0] = (u8)(word >> 0);
  tmp[1] = (u8)(word >> 8);
  tmp[2] = (u8)(word >> 16);
  tmp[3] = (u8)(word >> 24);
  return MIDIO_FILE_WriteBuffer(tmp, 4);
}


/////////////////////////////////////////////////////////////////////////////
// Creates a directory
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_MakeDir(char *path)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_MakeDir] ERROR: volume doesn't exist!\n");
#endif
    return MIDIO_FILE_ERR_NO_VOLUME;
  }

  if( (midio_file_dfs_errno=f_mkdir(path)) != FR_OK )
    return MIDIO_FILE_ERR_MKDIR;

  return 0; // directory created
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if file exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_FileExists(char *filepath)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_FileExists] ERROR: volume doesn't exist!\n");
#endif
    return MIDIO_FILE_ERR_NO_VOLUME;
  }

  if( f_open(&midio_file_read, filepath, FA_OPEN_EXISTING | FA_READ) != FR_OK )
    return 0; // file doesn't exist
  //f_close(&midio_file_read); // never close read files to avoid "invalid object"
  return 1; // file exists
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if directory exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_DirExists(char *path)
{
  DIR dir;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_DirExists] ERROR: volume doesn't exist!\n");
#endif
    return MIDIO_FILE_ERR_NO_VOLUME;
  }

  return (midio_file_dfs_errno=f_opendir(&dir, path)) == FR_OK;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if it is required to format any file
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_FormattingRequired(void)
{
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function formats Bank/Mixer/Song files
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_Format(void)
{
  s32 status = 0;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MIDIO_FILE_Format] ERROR: volume doesn't exist!\n");
#endif
    return MIDIO_FILE_ERR_NO_VOLUME;
  }

  if( midio_file_new_session_name[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MIDIO_FILE_Format] ERROR: no new session specified!\n");
#endif
    return MIDIO_FILE_ERR_NO_VOLUME;
  }

  // switch to new session name, store old name for the case that we have to switch back
  char prev_session_name[13];
  strcpy(prev_session_name, midio_file_session_name);
  strcpy(midio_file_session_name, midio_file_new_session_name); 


#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MIDIO_FILE_Format] Creating Session %s (previous was %s)\n", midio_file_session_name, prev_session_name);
#endif

  // in any case invalidate new session name
  midio_file_new_session_name[0] = 0;

  // not implemented yet...
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Send a verbose error message to MIOS terminal
// Used by SEQ_UI_SDCardErrMsg
/////////////////////////////////////////////////////////////////////////////
s32 MIDIO_FILE_SendErrorMessage(s32 error_status)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  switch( error_status ) {
  case MIDIO_FILE_ERR_SD_CARD: DEBUG_MSG("[SDCARD_ERROR:%d] failed to access SD card\n", error_status); break;
  case MIDIO_FILE_ERR_NO_PARTITION: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_GetPtnStart failed to find partition\n", error_status); break;
  case MIDIO_FILE_ERR_NO_VOLUME: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_GetVolInfo failed to find volume information\n", error_status); break;
  case MIDIO_FILE_ERR_UNKNOWN_FS: DEBUG_MSG("[SDCARD_ERROR:%d] unknown filesystem (only FAT12/16/32 supported)\n", error_status); break;
  case MIDIO_FILE_ERR_OPEN_READ: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenFile(..DFS_READ..) failed, e.g. file not found\n", error_status); break;
  case MIDIO_FILE_ERR_OPEN_READ_WITHOUT_CLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_ReadOpen() has been called while previous file hasn't been closed\n", error_status); break;
  case MIDIO_FILE_ERR_READ: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_ReadFile failed\n", error_status); break;
  case MIDIO_FILE_ERR_READCOUNT: DEBUG_MSG("[SDCARD_ERROR:%d] less bytes read than expected\n", error_status); break;
  case MIDIO_FILE_ERR_READCLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_ReadClose aborted due to previous error\n", error_status); break;
  case MIDIO_FILE_ERR_WRITE_MALLOC: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_WriteOpen failed to allocate memory for write buffer\n", error_status); break;
  case MIDIO_FILE_ERR_OPEN_WRITE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenFile(..DFS_WRITE..) failed\n", error_status); break;
  case MIDIO_FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_WriteOpen() has been called while previous file hasn't been closed\n", error_status); break;
  case MIDIO_FILE_ERR_WRITE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_WriteFile failed\n", error_status); break;
  case MIDIO_FILE_ERR_WRITECOUNT: DEBUG_MSG("[SDCARD_ERROR:%d] less bytes written than expected\n", error_status); break;
  case MIDIO_FILE_ERR_WRITECLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_WriteClose aborted due to previous error\n", error_status); break;
  case MIDIO_FILE_ERR_SEEK: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_Seek() failed\n", error_status); break;
  case MIDIO_FILE_ERR_OPEN_DIR: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found\n", error_status); break;
  case MIDIO_FILE_ERR_COPY: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_Copy() failed\n", error_status); break;
  case MIDIO_FILE_ERR_COPY_NO_FILE: DEBUG_MSG("[SDCARD_ERROR:%d] source file doesn't exist\n", error_status); break;
  case MIDIO_FILE_ERR_NO_DIR: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_GetDirs() or MIDIO_FILE_GetFiles() failed because of missing directory\n", error_status); break;
  case MIDIO_FILE_ERR_NO_FILE: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_GetFiles() failed because of missing directory\n", error_status); break;
  case MIDIO_FILE_ERR_SYSEX_READ: DEBUG_MSG("[SDCARD_ERROR:%d] error while reading .syx file\n", error_status); break;
  case MIDIO_FILE_ERR_MKDIR: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_MakeDir() failed\n", error_status); break;
  case MIDIO_FILE_ERR_INVALID_SESSION_NAME: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_LoadSessionName()\n", error_status); break;
  case MIDIO_FILE_ERR_UPDATE_FREE: DEBUG_MSG("[SDCARD_ERROR:%d] MIDIO_FILE_UpdateFreeBytes()\n", error_status); break;

  default:
    // remaining errors just print the number
    DEBUG_MSG("[SDCARD_ERROR:%d] see midio_file.h for description\n", error_status);
  }
#endif

  return 0; // no error
}

