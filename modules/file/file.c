// $Id$
//! \defgroup FILE
//!
//! File access layer for FatFS
//!
//! Frontend functions to read/write files.\n
//! Optimized for Memory Size and Speed!
//!
//! For the whole application only single file handlers for read and write
//! operations are available. They are shared globally to save memory (because
//! each FatFs handler allocates more than 512 bytes to store the last read
//! sector)
//!
//! For read operations it is possible to re-open a file via a file_t reference
//! so that no directory access is required to find the first sector of the
//! file (again).
//!
//! NOTE: before accessing the SD Card, the upper level function should
//! synchronize with a SD Card semaphore!
//! E.g. (defined in tasks.h in various projects):
//!   MUTEX_SDCARD_TAKE; // to take the semaphore
//!   MUTEX_SDCARD_GIVE; // to release the semaphore
//!
//! \{
/* ==========================================================================
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

#include "file.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1

#ifndef DEBUG_MSG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// last error status returned by DFS
// can be used as additional debugging help if FILE_*ERR returned by function
u32 file_dfs_errno;

// for percentage display during copy operations
u8 file_copy_percentage;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 FILE_MountFS(void);


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
static FIL file_read;
static u8 file_read_is_open; // only for safety purposes
static FIL file_write;
static u8 file_write_is_open; // only for safety purposes

// SD Card status
static u8 sdcard_available;
static u8 volume_available;
static u32 volume_free_bytes;

static u8 status_msg_ctr;

// buffer for copy operations and SysEx sender
#define TMP_BUFFER_SIZE _MAX_SS
static u8 tmp_buffer[TMP_BUFFER_SIZE];


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 FILE_Init(u32 mode)
{
  file_read_is_open = 0;
  file_write_is_open = 0;
  sdcard_available = 0;
  volume_available = 0;
  volume_free_bytes = 0;

  // init SDCard access
  s32 error = MIOS32_SDCARD_Init(0);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE] SD Card interface initialized, status: %d\n", error);
#endif

  // for status message
  status_msg_ctr = 5;

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically to check the availability
//! of the SD Card
//! \return < 0 on errors (error codes are documented in file.h)
//! \return 1 if SD card has been connected
//! \return 2 if SD card has been disconnected
//! \return 3 if new files should be loaded, e.g. after startup or SD Card change
/////////////////////////////////////////////////////////////////////////////
s32 FILE_CheckSDCard(void)
{
  // check if SD card is available
  // High-speed access if SD card was previously available
  u8 prev_sdcard_available = sdcard_available;
  sdcard_available = MIOS32_SDCARD_CheckAvailable(prev_sdcard_available);

  if( sdcard_available && !prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE] SD Card has been connected!\n");
#endif

    s32 error = FILE_MountFS();
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE] Tried to mount file system, status: %d\n", error);
#endif

    if( error < 0 ) {
      // ensure that volume flagged as not available
      volume_available = 0;

      return error; // break here!
    }

    // status message after 3 seconds
    status_msg_ctr = 3;

    return 1; // SD card has been connected

  } else if( !sdcard_available && prev_sdcard_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE] SD Card disconnected!\n");
#endif
    volume_available = 0;

    return 2; // SD card has been disconnected
  }

  if( status_msg_ctr ) {
    if( !--status_msg_ctr )
      return 3;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Mount the file system
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 FILE_MountFS(void)
{
  FRESULT res;
  DIR dir;

  file_read_is_open = 0;
  file_write_is_open = 0;

  if( (res=f_mount(0, &fs)) != FR_OK ) {
    DEBUG_MSG("[FILE] Failed to mount SD Card - error status: %d\n", res);
    return -1; // error
  }

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[FILE] Failed to open root directory - error status: %d\n", res);
    return -2; // error
  }

  // TODO: read from master sector
  disk_label[0] = 0;

  volume_available = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function updates the number of free bytes by scanning the FAT for
//! unused clusters.\n
//! should be called before FILE_Volume* bytes are read
/////////////////////////////////////////////////////////////////////////////
s32 FILE_UpdateFreeBytes(void)
{
  FRESULT res;
  DIR dir;
  DWORD free_clust;

  if( (res=f_opendir(&dir, "/")) != FR_OK ) {
    DEBUG_MSG("[FILE_UpdateFreeBytes] f_opendir failed with status %d!\n", res);
    return FILE_ERR_UPDATE_FREE;
  }

  if( (res=f_getfree("/", &free_clust, &dir.fs)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FILE_UpdateFreeBytes] f_getfree failed with status %d!\n", res);
#endif
    return FILE_ERR_UPDATE_FREE;
  }

  volume_free_bytes = free_clust * fs.csize * 512;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! \return 1 if SD card available
/////////////////////////////////////////////////////////////////////////////
s32 FILE_SDCardAvailable(void)
{
  return sdcard_available;
}


/////////////////////////////////////////////////////////////////////////////
//! \return 1 if volume available
/////////////////////////////////////////////////////////////////////////////
s32 FILE_VolumeAvailable(void)
{
  return volume_available;
}


/////////////////////////////////////////////////////////////////////////////
//! \return number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 FILE_VolumeBytesFree(void)
{
  return volume_free_bytes;
}


/////////////////////////////////////////////////////////////////////////////
//! \return total number of available bytes
/////////////////////////////////////////////////////////////////////////////
u32 FILE_VolumeBytesTotal(void)
{
  return (fs.max_clust-2)*fs.csize * 512;
}


/////////////////////////////////////////////////////////////////////////////
//! \return volume label
/////////////////////////////////////////////////////////////////////////////
char *FILE_VolumeLabel(void)
{
  return disk_label;
}


/////////////////////////////////////////////////////////////////////////////
//! opens a file for reading
//! Note: to save memory, one a single file is allowed to be opened per
//! time - always use FILE_ReadClose() before opening a new file!
//! Use FILE_ReadReOpen() to continue reading
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadOpen(file_t* file, char *filepath)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE] Opening file '%s'\n", filepath);
#endif

  if( file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FILE] FAILURE: tried to open file '%s' for reading, but previous file hasn't been closed!\n", filepath);
#endif
    return FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  if( (file_dfs_errno=f_open(&file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE] Error opening file - try mounting the partition again\n");
#endif

    s32 error;
    if( (error = FILE_MountFS()) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[FILE] mounting failed with status: %d\n", error);
#endif
      return FILE_ERR_SD_CARD;
    }

    if( (file_dfs_errno=f_open(&file_read, filepath, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[FILE] Still not able to open file - giving up!\n");
#endif
      return FILE_ERR_OPEN_READ;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE] found '%s' of length %u\n", filepath, file_read.fsize);
#endif

  // store current file variables in file_t
  file->flag = file_read.flag;
  file->csect = file_read.csect;
  file->fptr = file_read.fptr;
  file->fsize = file_read.fsize;
  file->org_clust = file_read.org_clust;
  file->curr_clust = file_read.curr_clust;
  file->dsect = file_read.dsect;
  file->dir_sect = file_read.dir_sect;
  file->dir_ptr = file_read.dir_ptr;

  // file is opened
  file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! reopens a file for reading
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadReOpen(file_t* file)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE] Reopening file\n");
#endif

  if( file_read_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FILE] FAILURE: tried to reopen file, but previous file hasn't been closed!\n");
#endif
    return FILE_ERR_OPEN_READ_WITHOUT_CLOSE;
  }

  // restore file variables from file_t
  file_read.fs = &fs;
  file_read.id = fs.id;
  file_read.flag = file->flag;
  file_read.csect = file->csect;
  file_read.fptr = file->fptr;
  file_read.fsize = file->fsize;
  file_read.org_clust = file->org_clust;
  file_read.curr_clust = file->curr_clust;
  file_read.dsect = file->dsect;
  file_read.dir_sect = file->dir_sect;
  file_read.dir_ptr = file->dir_ptr;

  // ensure that the right sector is in cache again
  disk_read(file_read.fs->drive, file_read.buf, file_read.dsect, 1);

  // file is opened (again)
  file_read_is_open = 1;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Closes a file which has been read
//! File can be re-opened if required thereafter w/o performance issues
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadClose(file_t *file)
{
  // store current file variables in file_t
  file->flag = file_read.flag;
  file->csect = file_read.csect;
  file->fptr = file_read.fptr;
  file->fsize = file_read.fsize;
  file->org_clust = file_read.org_clust;
  file->curr_clust = file_read.curr_clust;
  file->dsect = file_read.dsect;
  file->dir_sect = file_read.dir_sect;
  file->dir_ptr = file_read.dir_ptr;

  // file has been closed
  file_read_is_open = 0;


  // don't close file via f_close()! We allow to open the file again
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Changes to a new file position
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadSeek(u32 offset)
{
  if( (file_dfs_errno=f_lseek(&file_read, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, file_dfs_errno);
#endif
    return FILE_ERR_SEEK;
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Read from file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadBuffer(u8 *buffer, u32 len)
{
  UINT successcount;

  // exit if volume not available
  if( !volume_available )
    return FILE_ERR_NO_VOLUME;

  if( (file_dfs_errno=f_read(&file_read, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[FILE] Failed to read sector at position 0x%08x, status: %u\n", file_read.fptr, file_dfs_errno);
#endif
      return FILE_ERR_READ;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[FILE] Wrong successcount while reading from position 0x%08x (count: %d)\n", file_read.fptr, successcount);
#endif
    return FILE_ERR_READCOUNT;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Read a string (terminated with CR) from file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadLine(u8 *buffer, u32 max_len)
{
  s32 status;
  u32 num_read = 0;

  while( file_read.fptr < file_read.fsize ) {
    status = FILE_ReadBuffer(buffer, 1);

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

/////////////////////////////////////////////////////////////////////////////
//! Read a 8bit value from file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadByte(u8 *byte)
{
  return FILE_ReadBuffer(byte, 1);
}

/////////////////////////////////////////////////////////////////////////////
//! Read a 16bit value from file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadHWord(u16 *hword)
{
  // ensure little endian coding
  u8 tmp[2];
  s32 status = FILE_ReadBuffer(tmp, 2);
  *hword = ((u16)tmp[0] << 0) | ((u16)tmp[1] << 8);
  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Read a 32bit value from file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_ReadWord(u32 *word)
{
  // ensure little endian coding
  u8 tmp[4];
  s32 status = FILE_ReadBuffer(tmp, 4);
  *word = ((u32)tmp[0] << 0) | ((u32)tmp[1] << 8) | ((u32)tmp[2] << 16) | ((u32)tmp[3] << 24);
  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Opens a file for writing
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteOpen(char *filepath, u8 create)
{
  if( file_write_is_open ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FILE] FAILURE: tried to open file '%s' for writing, but previous file hasn't been closed!\n", filepath);
#endif
    return FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE;
  }

  if( (file_dfs_errno=f_open(&file_write, filepath, (create ? FA_CREATE_ALWAYS : FA_OPEN_EXISTING) | FA_WRITE)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE] error opening file '%s' for writing!\n", filepath);
#endif
    return FILE_ERR_OPEN_WRITE;
  }

  // remember state
  file_write_is_open = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Closes a file by writing the last bytes
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteClose(void)
{
  s32 status = 0;

  // close file
  if( (file_dfs_errno=f_close(&file_write)) != FR_OK )
    status = FILE_ERR_WRITECLOSE;

  file_write_is_open = 0;

  return status;
}



/////////////////////////////////////////////////////////////////////////////
//! Changes to a new file position
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteSeek(u32 offset)
{
  if( (file_dfs_errno=f_lseek(&file_write, offset)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_ReadSeek] ERROR: seek to offset %u failed (FatFs status: %d)\n", offset, file_dfs_errno);
#endif
    return FILE_ERR_SEEK;
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns current size of write file
/////////////////////////////////////////////////////////////////////////////
u32 FILE_WriteGetCurrentSize(void)
{
  if( !file_write_is_open )
    return 0;
  return file_write.fsize;
}


/////////////////////////////////////////////////////////////////////////////
//! Writes into a file with caching mechanism (actual write at end of sector)
//! File has to be closed via FILE_WriteClose() after the last byte has
//! been written
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteBuffer(u8 *buffer, u32 len)
{
  // exit if volume not available
  if( !volume_available )
    return FILE_ERR_NO_VOLUME;

  UINT successcount;
  if( (file_dfs_errno=f_write(&file_write, buffer, len, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[FILE] Failed to write buffer, status: %u\n", file_dfs_errno);
#endif
    return FILE_ERR_WRITE;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[FILE] Wrong successcount while writing buffer (count: %d)\n", successcount);
#endif
    return FILE_ERR_WRITECOUNT;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Writes a 8bit value into file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteByte(u8 byte)
{
  return FILE_WriteBuffer(&byte, 1);
}

/////////////////////////////////////////////////////////////////////////////
//! Writes a 16bit value into file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteHWord(u16 hword)
{
  // ensure little endian coding
  u8 tmp[2];
  tmp[0] = (u8)(hword >> 0);
  tmp[1] = (u8)(hword >> 8);
  return FILE_WriteBuffer(tmp, 2);
}

/////////////////////////////////////////////////////////////////////////////
//! Writes a 32bit value into file
//! \return < 0 on errors (error codes are documented in file.h)
/////////////////////////////////////////////////////////////////////////////
s32 FILE_WriteWord(u32 word)
{
  // ensure little endian coding
  u8 tmp[4];
  tmp[0] = (u8)(word >> 0);
  tmp[1] = (u8)(word >> 8);
  tmp[2] = (u8)(word >> 16);
  tmp[3] = (u8)(word >> 24);
  return FILE_WriteBuffer(tmp, 4);
}


/////////////////////////////////////////////////////////////////////////////
//! This function copies a file
//! \param[in] src_file the source file which should be copied
//! \param[in] dst_file the destination file
/////////////////////////////////////////////////////////////////////////////
s32 FILE_Copy(char *src_file, char *dst_file)
{
  s32 status = 0;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_Copy] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE_Copy] copy %s to %s\n", src_file, dst_file);
#endif

  if( (file_dfs_errno=f_open(&file_read, src_file, FA_OPEN_EXISTING | FA_READ)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_Copy] %s doesn't exist!\n", src_file);
#endif
    status = FILE_ERR_COPY_NO_FILE;
  } else {
    if( (file_dfs_errno=f_open(&file_write, dst_file, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[FILE_Copy] wasn't able to create %s - exit!\n", dst_file);
#endif
      status = FILE_ERR_COPY;
      //f_close(&file_read); // never close read files to avoid "invalid object"
    }
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_Copy] Copy aborted due to previous errors (%d)!\n", status);
#endif
  } else {
    // copy operation
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_Copy] Starting copy operation!\n");
#endif

    file_copy_percentage = 0; // for percentage display

    UINT successcount;
    UINT successcount_wr;
    u32 num_bytes = 0;
    do {
      if( (file_dfs_errno=f_read(&file_read, tmp_buffer, TMP_BUFFER_SIZE, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[FILE_Copy] Failed to read sector at position 0x%08x, status: %u\n", file_read.fptr, file_dfs_errno);
#endif
	successcount = 0;
	status = FILE_ERR_READ;
      } else if( successcount && f_write(&file_write, tmp_buffer, successcount, &successcount_wr) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[FILE_Copy] Failed to write sector at position 0x%08x, status: %u\n", file_write.fptr, file_dfs_errno);
#endif
	status = FILE_ERR_WRITE;
      } else {
	num_bytes += successcount_wr;
	file_copy_percentage = (u8)((100 * num_bytes) / file_read.fsize);
      }
    } while( status == 0 && successcount > 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_Copy] Finished copy operation (%d bytes)!\n", num_bytes);
#endif

    //f_close(&file_read); // never close read files to avoid "invalid object"
    f_close(&file_write);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Creates a directory
/////////////////////////////////////////////////////////////////////////////
s32 FILE_MakeDir(char *path)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_MakeDir] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( (file_dfs_errno=f_mkdir(path)) != FR_OK )
    return FILE_ERR_MKDIR;

  return 0; // directory created
}


/////////////////////////////////////////////////////////////////////////////
//! Returns 1 if file exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FILE_FileExists(char *filepath)
{
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_FileExists] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( f_open(&file_read, filepath, FA_OPEN_EXISTING | FA_READ) != FR_OK )
    return 0; // file doesn't exist
  //f_close(&file_read); // never close read files to avoid "invalid object"
  return 1; // file exists
}


/////////////////////////////////////////////////////////////////////////////
//! Returns 1 if directory exists, 0 if it doesn't exist, < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 FILE_DirExists(char *path)
{
  DIR dir;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_DirExists] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  return (file_dfs_errno=f_opendir(&dir, path)) == FR_OK;
}


/////////////////////////////////////////////////////////////////////////////
// This function searches for directories under given path and copies the names
// into a list (e.g. used by seq_ui_sysex.c)
// !!TEMPORARY VERSION!! will be combined with FILE_GetFiles later
/////////////////////////////////////////////////////////////////////////////
s32 FILE_GetDirs(char *path, char *dir_list, u8 num_of_items, u8 dir_offset)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_GetDirs] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_GetDirs] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = FILE_ERR_NO_DIR;
  }

  int num_dirs = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' && (de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {
      ++num_dirs;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif

      if( num_dirs > dir_offset && num_dirs <= (dir_offset+num_of_items) ) {
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
// !!TEMPORARY VERSION!! will be combined with FILE_GetDirs later
/////////////////////////////////////////////////////////////////////////////
s32 FILE_GetFiles(char *path, char *ext_filter, char *file_list, u8 num_of_items, u8 file_offset)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_GetFiles] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_GetFiles] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = FILE_ERR_NO_DIR;
  }

  int num_files = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' &&
	!(de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {

      char *p = (char *)&de.fname[0];
      int i;
      for(i=0; i<9; ++i, p++) {
	if( *p == '.' )
	  break;
      }

      if( ext_filter && *p++ != '.' )
	continue;

      if( ext_filter && strncasecmp(p, ext_filter, 3) != 0 )
	continue;

      ++num_files;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif
      if( num_files > file_offset && num_files <= (file_offset+num_of_items) ) {
	int item_pos = 9 * (num_files-1-file_offset);
	char *p = (char *)&file_list[item_pos];

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
//! Returns a file next to the given filename with the given extension
//! if filename == NULL, the first file will be returned in next_filename
//! \param[in] path directory in which we want to search
//! \param[in] filename current file (if NULL, the first file will be returned)
//! \param[in] ext_filter optional extension filter (if NULL: no extension)
//! \param[out] next_filename the next file if return status is 1
//! \return < 0 on errors (error codes are documented in file.h)
//! \return 1 if next file has been found
//! \return 0 if no additional file
/////////////////////////////////////////////////////////////////////////////
s32 FILE_FindNextFile(char *path, char *filename, char *ext_filter, char *next_filename)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_FindNextFile] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_FindNextFile] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = FILE_ERR_NO_DIR;
  }

  u8 take_next = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' &&
	!(de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {

      char *p = (char *)&de.fname[0];
      int i;
      u8 filename_matches = 1;
      char *p_check = filename;
      for(i=0; i<9; ++i, p++) {
	if( p_check && *p != *p_check++ ) {
	  filename_matches = 0;
	}

	if( *p == '.' )
	  break;
      }

      if( ext_filter && *p++ != '.' )
	continue;

      if( ext_filter && strncasecmp(p, ext_filter, 3) != 0 )
	continue;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif

      if( take_next || filename == NULL ) {
	memcpy((char *)next_filename, (char *)&de.fname[0], 13);
	return 1; // file found
      } else if( filename_matches ) {
	take_next = 1;
      }
    }
  }

  next_filename[0] = 0; // set terminator (empty string)
  return 0; // file not found
}


/////////////////////////////////////////////////////////////////////////////
//! Returns a previous file of the given filename with the given extension
//! if filename == NULL, no first file will be returned in prev_filename
//! \param[in] path directory in which we want to search
//! \param[in] filename current file (if NULL, the first file will be returned)
//! \param[in] ext_filter optional extension filter (if NULL: no extension)
//! \param[out] prev_filename the previous file if return status is 1
//! \return < 0 on errors (error codes are documented in file.h)
//! \return 1 if previous file has been found
//! \return 0 if no additional file
/////////////////////////////////////////////////////////////////////////////
s32 FILE_FindPreviousFile(char *path, char *filename, char *ext_filter, char *prev_filename)
{
  s32 status = 0;
  DIR di;
  FILINFO de;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_FindPreviousFile] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( f_opendir(&di, path) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_FindPreviousFile] ERROR: opening %s directory - please create it!\n", path);
#endif
    status = FILE_ERR_NO_DIR;
  }

  prev_filename[0] = 0;
  while( status == 0 && f_readdir(&di, &de) == FR_OK && de.fname[0] != 0 ) {
    if( de.fname[0] && de.fname[0] != '.' &&
	!(de.fattrib & AM_DIR) && !(de.fattrib & AM_HID) ) {

      char *p = (char *)&de.fname[0];
      int i;
      u8 filename_matches = 1;
      char *p_check = filename;
      for(i=0; i<9; ++i, p++) {
	if( p_check && *p != *p_check++ ) {
	  filename_matches = 0;
	}

	if( *p == '.' )
	  break;
      }

      if( ext_filter && *p++ != '.' )
	continue;

      if( ext_filter && strncasecmp(p, ext_filter, 3) != 0 )
	continue;

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("--> %s\n", de.fname);
#endif

      if( filename_matches ) {
	if( p_check == NULL ) // first file
	  memcpy((char *)prev_filename, (char *)&de.fname[0], 13);
	return (prev_filename[0] == 0) ? 0 : 1;
      }

      // copy filename into prev_filename for next iteration
      memcpy((char *)prev_filename, (char *)&de.fname[0], 13);
    }
  }

  prev_filename[0] = 0; // set terminator (empty string)
  return 0; // file not found
}


/////////////////////////////////////////////////////////////////////////////
//! This function sends a .syx file to given MIDI out port
/////////////////////////////////////////////////////////////////////////////
s32 FILE_SendSyxDump(char *path, mios32_midi_port_t port)
{
  s32 status = 0;
  file_t file;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_SendSyxDump] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE_SendSyxDump] Open file '%s'\n", path);
#endif

  if( (status=FILE_ReadOpen(&file, path)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[FILE_SendSyxDump] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // stream SysEx to MIDI port
  UINT successcount;
  u32 num_bytes = 0;
  do {
    if( (file_dfs_errno=f_read(&file_read, tmp_buffer, TMP_BUFFER_SIZE, &successcount)) != FR_OK ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[FILE] Failed to read sector at position 0x%08x, status: %u\n", file_read.fptr, file_dfs_errno);
#endif
      successcount = 0;
      status = FILE_ERR_SYSEX_READ;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      MIOS32_MIDI_SendDebugHexDump(tmp_buffer, successcount);
#endif
      MIOS32_MIDI_SendSysEx(port, tmp_buffer, successcount);
      num_bytes += successcount;
    }
  } while( status == 0 && successcount > 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[FILE_SendSyxDump] sent %d bytes!\n", num_bytes);
#endif

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[FILE_SendSyxDump] ERROR while reading file, status: %d\n", status);
#endif
    return FILE_ERR_SYSEX_READ;
  }

  return num_bytes;
}


/////////////////////////////////////////////////////////////////////////////
//! This function prints some useful SD card informations on the MIOS terminal
/////////////////////////////////////////////////////////////////////////////
s32 FILE_PrintSDCardInfos(void)
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
//! Send a verbose error message to MIOS terminal
/////////////////////////////////////////////////////////////////////////////
s32 FILE_SendErrorMessage(s32 error_status)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  switch( error_status ) {
  case FILE_ERR_SD_CARD: DEBUG_MSG("[SDCARD_ERROR:%d] failed to access SD card\n", error_status); break;
  case FILE_ERR_NO_PARTITION: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_GetPtnStart failed to find partition\n", error_status); break;
  case FILE_ERR_NO_VOLUME: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_GetVolInfo failed to find volume information\n", error_status); break;
  case FILE_ERR_UNKNOWN_FS: DEBUG_MSG("[SDCARD_ERROR:%d] unknown filesystem (only FAT12/16/32 supported)\n", error_status); break;
  case FILE_ERR_OPEN_READ: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenFile(..DFS_READ..) failed, e.g. file not found\n", error_status); break;
  case FILE_ERR_OPEN_READ_WITHOUT_CLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_ReadOpen() has been called while previous file hasn't been closed\n", error_status); break;
  case FILE_ERR_READ: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_ReadFile failed\n", error_status); break;
  case FILE_ERR_READCOUNT: DEBUG_MSG("[SDCARD_ERROR:%d] less bytes read than expected\n", error_status); break;
  case FILE_ERR_READCLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_ReadClose aborted due to previous error\n", error_status); break;
  case FILE_ERR_WRITE_MALLOC: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_WriteOpen failed to allocate memory for write buffer\n", error_status); break;
  case FILE_ERR_OPEN_WRITE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenFile(..DFS_WRITE..) failed\n", error_status); break;
  case FILE_ERR_OPEN_WRITE_WITHOUT_CLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_WriteOpen() has been called while previous file hasn't been closed\n", error_status); break;
  case FILE_ERR_WRITE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_WriteFile failed\n", error_status); break;
  case FILE_ERR_WRITECOUNT: DEBUG_MSG("[SDCARD_ERROR:%d] less bytes written than expected\n", error_status); break;
  case FILE_ERR_WRITECLOSE: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_WriteClose aborted due to previous error\n", error_status); break;
  case FILE_ERR_SEEK: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_Seek() failed\n", error_status); break;
  case FILE_ERR_OPEN_DIR: DEBUG_MSG("[SDCARD_ERROR:%d] DFS_OpenDir(..DFS_READ..) failed, e.g. directory not found\n", error_status); break;
  case FILE_ERR_COPY: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_Copy() failed\n", error_status); break;
  case FILE_ERR_COPY_NO_FILE: DEBUG_MSG("[SDCARD_ERROR:%d] source file doesn't exist\n", error_status); break;
  case FILE_ERR_NO_DIR: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_GetDirs() or FILE_GetFiles() failed because of missing directory\n", error_status); break;
  case FILE_ERR_NO_FILE: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_GetFiles() failed because of missing directory\n", error_status); break;
  case FILE_ERR_SYSEX_READ: DEBUG_MSG("[SDCARD_ERROR:%d] error while reading .syx file\n", error_status); break;
  case FILE_ERR_MKDIR: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_MakeDir() failed\n", error_status); break;
  case FILE_ERR_INVALID_SESSION_NAME: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_LoadSessionName()\n", error_status); break;
  case FILE_ERR_UPDATE_FREE: DEBUG_MSG("[SDCARD_ERROR:%d] FILE_UpdateFreeBytes()\n", error_status); break;

  default:
    // remaining errors just print the number
    DEBUG_MSG("[SDCARD_ERROR:%d] see file.h for description\n", error_status);
  }
#endif

  return 0; // no error
}


//! \}
