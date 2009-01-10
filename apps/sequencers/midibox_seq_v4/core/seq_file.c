// $Id$
/*
 * File access functions
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

#include <dosfs.h>
#include <string.h>

#include "seq_file.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging via COM interface
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0

// add following lines to your mios32_config.h file to send these messages via UART1
// // enable COM via UART1
// #define MIOS32_UART1_ASSIGNMENT 2
// #define MIOS32_UART1_BAUDRATE 115200
// #define MIOS32_COM_DEFAULT_PORT UART1


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// last error status returned by DFS
// can be used as additional debugging help if SEQ_FILE_*ERR returned by function
u32 seq_file_dfs_errno;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_FILE_MountFS(void);
static s32 SEQ_FILE_UpdateFreeBytes(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// DOS FS variables
static u8 sector[SECTOR_SIZE];

static u8 volume_available;
static u32 volume_free_bytes;

static VOLINFO vi;

static s32 write_filepos;
static u8 *write_buffer;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Init(u32 mode)
{
  volume_available = 0;
  volume_free_bytes = 0;

  // init SDCard access
  s32 error = MIOS32_SDCARD_Init(0);
#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE] SD Card initialized, status: %d\n\r", error);
#endif

  if( error >= 0 ) {
    error = MIOS32_SDCARD_PowerOn();
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] SD Card power on sequence, status: %d\n\r", error);
#endif
  }

  if( error >= 0 ) {
    error = SEQ_FILE_MountFS();
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] Tried to mount file system, status: %d\n\r", error);
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Mount the file system
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_MountFS(void)
{
  u32 pstart, psize;
  u8  pactive, ptype;

  // Obtain pointer to first partition on first (only) unit
  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
  if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] Cannot find first partition - reconnect SD Card\n\r");
#endif

    s32 error = MIOS32_SDCARD_PowerOn();
    if( error < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[SEQ_FILE] Failed - no access to SD Card\n\r");
#endif
      return SEQ_FILE_ERR_SD_CARD;
    }

    pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
    if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[SEQ_FILE] SD Card available, but still cannot find first partition - giving up!\n\r");
#endif
      return SEQ_FILE_ERR_NO_PARTITION;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[SEQ_FILE] Partition 0 start sector 0x%-08.8lX active %-02.2hX type %-02.2hX size %-08.8lX\n\r", pstart, pactive, ptype, psize);
#endif

  if( seq_file_dfs_errno = DFS_GetVolInfo(0, sector, pstart, &vi) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] Error getting volume information\n\r");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[SEQ_FILE] Volume label '%s'\n\r", vi.label);
  printf("[SEQ_FILE] %d sector/s per cluster, %d reserved sector/s, volume total %d sectors.\n\r", vi.secperclus, vi.reservedsecs, vi.numsecs);
  printf("[SEQ_FILE] %d sectors per FAT, first FAT at sector #%d, root dir at #%d.\n\r",vi.secperfat,vi.fat1,vi.rootdir);
  printf("[SEQ_FILE] (For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n\r");
  printf("[SEQ_FILE] %d root dir entries, data area commences at sector #%d.\n\r",vi.rootentries,vi.dataarea);
  printf("[SEQ_FILE] %d clusters (%d bytes) in data area, filesystem IDd as ", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE);
  if (vi.filesystem == FAT12)
    printf("FAT12.\n\r");
  else if (vi.filesystem == FAT16)
    printf("FAT16.\n\r");
  else if (vi.filesystem == FAT32)
    printf("FAT32.\n\r");
  else {
    printf("[SEQ_FILE] [unknown]\n\r");
  }
#endif

  if( vi.filesystem != FAT12 && vi.filesystem != FAT16 && vi.filesystem != FAT32 )
    return SEQ_FILE_ERR_UNKNOWN_FS;
  else {
    volume_available = 1;
    SEQ_FILE_UpdateFreeBytes();
  }
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the number of free bytes by scanning the FAT for
// unused clusters.
// should be called whenever a new file has been created!
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_FILE_UpdateFreeBytes(void)
{
  volume_free_bytes = 0xffffffff;

  // Disabled: takes too long! (e.g. my SD card has ca. 2000 clusters, 
  // it takes ca. 3 seconds to determine free clusters)
#if 0
  // exit if volume not available
  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[SEQ_FILE] UpdateFreeBytes stopped - no volume!\n\r");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

  u32 scratchcache = 0;
  u32 i;

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE] Starting UpdateFreeBytes, scan for %u clusters\n\r", vi.numclusters);
#endif

  for(i=2; i<vi.numclusters; ++i) {
    if( !DFS_GetFAT(&vi, sector, &scratchcache, i) )
      volume_free_bytes += vi.secperclus * SECTOR_SIZE;
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE] Finished UpdateFreeBytes: %u\n\r", volume_free_bytes);
#endif
#endif

  return 0; // no error
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
  return vi.numclusters * vi.secperclus * SECTOR_SIZE;
}


/////////////////////////////////////////////////////////////////////////////
// opens a file for reading
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadOpen(PFILEINFO fileinfo, char *filepath)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE] Opening file '%s'\n\r", filepath);
#endif

  // enable caching
  // attention:  this feature has to be explicitely enabled, as it isn't reentrant
  // and requires to use the same buffer pointer whenever reading a file.
  DFS_CachingEnabledSet(1);

  if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_READ, sector, fileinfo) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] Error opening file - try mounting the partition again\n\r");
#endif

    s32 error;
    if( (error = SEQ_FILE_MountFS()) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[SEQ_FILE] mounting failed with status: %d\n\r", error);
#endif
      return SEQ_FILE_ERR_SD_CARD;
    }

    if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_READ, sector, fileinfo) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[SEQ_FILE] Still not able to open file - giving up!\n\r");
#endif
      return SEQ_FILE_ERR_OPEN_READ;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE] found '%s' of length %u\n\r", filepath, fileinfo->filelen);
#endif

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Read from file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len)
{
  u32 successcount;

  // exit if volume not available
  if( !volume_available )
    return SEQ_FILE_ERR_NO_VOLUME;

  if( seq_file_dfs_errno = DFS_ReadFile(fileinfo, sector, buffer, &successcount, len) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n\r", fileinfo->pointer, seq_file_dfs_errno);
#endif
      return SEQ_FILE_ERR_READ;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[SEQ_FILE] Wrong successcount while reading from position 0x%08x (count: %d)\n\r", fileinfo->pointer, successcount);
#endif
    return SEQ_FILE_ERR_READCOUNT;
  }

  return 0; // no error
}

s32 SEQ_FILE_ReadByte(PFILEINFO fileinfo, u8 *byte)
{
  s32 status = 0;
  status |= SEQ_FILE_ReadBuffer(fileinfo, byte, 1);
  return status;
}

s32 SEQ_FILE_ReadHWord(PFILEINFO fileinfo, u16 *hword)
{
  s32 status = 0;
  // ensure little endian coding, therefore 2 reads
  *hword = 0x0000;
  u8 tmp;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *hword |= (u16)tmp << 0;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *hword |= (u16)tmp << 8;
  return status;
}

s32 SEQ_FILE_ReadWord(PFILEINFO fileinfo, u32 *word)
{
  s32 status = 0;
  // ensure little endian coding, therefore 4 reads
  *word = 0x00000000;
  u8 tmp;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *word |= (u16)tmp << 0;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *word |= (u16)tmp << 8;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *word |= (u16)tmp << 16;
  status |= SEQ_FILE_ReadBuffer(fileinfo, &tmp, 1);
  *word |= (u16)tmp << 24;
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file for writing
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteOpen(PFILEINFO fileinfo, char *filepath, u8 create)
{
  // reset filepos
  write_filepos = 0;

  // try to allocate buffer for write sector
  write_buffer = (u8 *)pvPortMalloc(SECTOR_SIZE);
  if( write_buffer == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] failed to allocate %d bytes for write sector\n\r", SECTOR_SIZE);
#endif
    return SEQ_FILE_ERR_WRITE_MALLOC;
  }

  // it's better to disable caching here, since different sectors are accessed
  DFS_CachingEnabledSet(0);

  if( create && (seq_file_dfs_errno = DFS_UnlinkFile(&vi, filepath, sector)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[SEQ_FILE] couldn't delete '%s' (file not found...)\n\r", filepath);
#endif
    // continue .. perhaps it didn't exist
  }
  
  if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_WRITE, sector, fileinfo) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE] error opening file '%s' for writing!\n\r", filepath);
#endif
    // free memory
    vPortFree(write_buffer);

    // enable cache again
    DFS_CachingEnabledSet(1);

    return SEQ_FILE_ERR_OPEN_WRITE;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Writes into a file with caching mechanism (actual write at end of sector)
// File has to be closed via SEQ_FILE_WriteClose() after the last byte has
// been written
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteBuffer(PFILEINFO fileinfo, u8 *buffer, u32 len)
{
  // exit if volume not available
  if( !volume_available )
    return SEQ_FILE_ERR_NO_VOLUME;

  // exit if filepos < 0 -> this indicates that an error has happened during previous write operation
  if( write_filepos < 0 )
    return write_filepos; // skipped due to previous error

  // will sector boundary be reached? Then we have to write the sector
  while( ((write_filepos % SECTOR_SIZE) + len) >= SECTOR_SIZE ) {
    // fill up the write buffer
    u32 bytes_to_add = SECTOR_SIZE - (write_filepos % SECTOR_SIZE);
    memcpy(write_buffer+(write_filepos % SECTOR_SIZE), buffer, bytes_to_add);

    // write sector
    u32 successcount;
    if( seq_file_dfs_errno = DFS_WriteFile(fileinfo, sector, write_buffer, &successcount, SECTOR_SIZE) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n\r", write_filepos, seq_file_dfs_errno);
#endif
      write_filepos = -1; // ensure that next writes will be skipped
      return SEQ_FILE_ERR_WRITE;
    }
    if( successcount != SECTOR_SIZE ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[SEQ_FILE] Wrong successcount while writing to position 0x%08x (count: %d)\n\r", write_filepos, successcount);
#endif
      write_filepos = -2; // ensure that next writes will be skipped
      return SEQ_FILE_ERR_WRITECOUNT;
    }

    write_filepos += bytes_to_add;
    len -= bytes_to_add;
    buffer += bytes_to_add;
    // we loop until the remaining bytes are less than sector size
  }

  // still something to add?
  if( len ) {
    if( len == 1 )
      write_buffer[write_filepos % SECTOR_SIZE] = *buffer; // should be faster than memcpy
    else
      memcpy(write_buffer+(write_filepos % SECTOR_SIZE), buffer, len); // usually assembly optimized...
    write_filepos += len;
  }

  return 0; // no error
}

s32 SEQ_FILE_WriteByte(PFILEINFO fileinfo, u8 byte)
{
  s32 status = 0;
  status |= SEQ_FILE_WriteBuffer(fileinfo, &byte, 1);
  return status;
}

s32 SEQ_FILE_WriteHWord(PFILEINFO fileinfo, u16 hword)
{
  s32 status = 0;
  // ensure little endian coding, therefore 2 writes
  u8 tmp;
  tmp = (u8)(hword >> 0);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  tmp = (u8)(hword >> 8);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  return status;
}

s32 SEQ_FILE_WriteWord(PFILEINFO fileinfo, u32 word)
{
  s32 status = 0;
  // ensure little endian coding, therefore 4 writes
  u8 tmp;
  tmp = (u8)(word >> 0);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  tmp = (u8)(word >> 8);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  tmp = (u8)(word >> 16);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  tmp = (u8)(word >> 24);
  status |= SEQ_FILE_WriteBuffer(fileinfo, &tmp, 1);
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Closes a file by writing the last bytes
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteClose(PFILEINFO fileinfo)
{
  u32 status = 0;
  u32 successcount;

  // don't write if filepos < 0 -> this indicates that an error has happened during previous write operation
  if( write_filepos < 0 ) {
    status = 3; // skipped due to previous error
  } else {
    u32 len = (write_filepos % SECTOR_SIZE);

    if( len && (seq_file_dfs_errno=DFS_WriteFile(fileinfo, sector, write_buffer, &successcount, len)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n\r", write_filepos-SECTOR_SIZE, seq_file_dfs_errno);
#endif

      status = -1; // write error
    } else {
      if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	printf("[SEQ_FILE] Wrong successcount while writing to position 0x%08x (count: %d)\n\r", write_filepos-SECTOR_SIZE, successcount);
#endif
	
	status = -2; // not all bytes written
      }
    }
  }

  // free memory
  vPortFree(write_buffer);

  // enable cache again
  DFS_CachingEnabledSet(1);

  // update number of free bytes
  SEQ_FILE_UpdateFreeBytes();

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Changes to a new file position
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Seek(PFILEINFO fileinfo, u32 offset)
{
  DFS_Seek(fileinfo, offset, sector);

  return (fileinfo->pointer == offset) ? 0 : SEQ_FILE_ERR_SEEK;
}
