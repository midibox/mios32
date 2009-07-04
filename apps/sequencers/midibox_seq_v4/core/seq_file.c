// $Id$
/*
 * File access functions
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

#include <dosfs.h>
#include <string.h>

#include "seq_ui.h"

#include "seq_file.h"

#include "seq_file_b.h"
#include "seq_file_m.h"
#include "seq_file_s.h"
#include "seq_file_g.h"
#include "seq_file_c.h"
#include "seq_file_hw.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 2


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are backup directories located?
// recommented: "backup/"
// backup subdirs (backup/1, backup/2, backup/3, ..." have to be manually
// created, since DosFS doesn't support the creation of new dirs

#define SEQ_FILE_BACKUP_PATH "backup/"


// allows to enable malloc instead of static allocation of write buffer
#define SEQ_FILE_WRITE_BUFFER_MALLOC 0


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
static s32 SEQ_FILE_UpdateFreeBytes(void);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// DOS FS variables
static u8 sector[SECTOR_SIZE];

static u8 sdcard_available;
static u8 volume_available;
static u32 volume_free_bytes;

static VOLINFO vi;

static s32 write_filepos;

#if SEQ_FILE_WRITE_BUFFER_MALLOC
static u8 *write_buffer;
#else
static u8 write_buffer[SECTOR_SIZE];
#endif

static u8 status_msg_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Init(u32 mode)
{
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
  u32 pstart, psize;
  u8  pactive, ptype;

  // Obtain pointer to first partition on first (only) unit
  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
  if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] Cannot find first partition - reconnect SD Card\n");
#endif

    s32 error = MIOS32_SDCARD_PowerOn();
    if( error < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] Failed - no access to SD Card\n");
#endif
      return SEQ_FILE_ERR_SD_CARD;
    }

    pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);

    if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] SD Card available, but still cannot find first partition - giving up!\n");
#endif
      return SEQ_FILE_ERR_NO_PARTITION;
    }
  }

  // check for partition type, if we don't get one of these types, it can be assumed that the partition
  // is located at the first sector instead MBR ("superfloppy format")
  // see also http://mirror.href.com/thestarman/asm/mbr/PartTypes.htm
  if( ptype != 0x04 && ptype != 0x06 && ptype != 0x0b && ptype != 0x0c && ptype != 0x0e ) {
    pstart = 0;
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Partition 0 start sector %u (invalid type, assuming superfloppy format)\n", pstart);
#endif
  } else {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Partition 0 start sector %u active 0x%02x type 0x%02x size %u\n", pstart, pactive, ptype, psize);
#endif
  }

  if( seq_file_dfs_errno = DFS_GetVolInfo(0, sector, pstart, &vi) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] Error getting volume information\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 3
  DEBUG_MSG("[SEQ_FILE] Volume label '%s'\n", vi.label);
  DEBUG_MSG("[SEQ_FILE] %u sector/s per cluster, %u reserved sector/s, volume total %u sectors.\n", vi.secperclus, vi.reservedsecs, vi.numsecs);
  DEBUG_MSG("[SEQ_FILE] %u sectors per FAT, first FAT at sector #%u, root dir at #%u.\n",vi.secperfat,vi.fat1,vi.rootdir);
  DEBUG_MSG("[SEQ_FILE] (For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n");
  DEBUG_MSG("[SEQ_FILE] %u root dir entries, data area commences at sector #%u.\n",vi.rootentries,vi.dataarea);
  char file_system[20];
  if( vi.filesystem == FAT12 )
    strcpy(file_system, "FAT12");
  else if (vi.filesystem == FAT16)
    strcpy(file_system, "FAT16");
  else if (vi.filesystem == FAT32)
    strcpy(file_system, "FAT32");
  else
    strcpy(file_system, "unknown FS");
  DEBUG_MSG("[SEQ_FILE] %u clusters (%u bytes) in data area, filesystem IDd as %s", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE, file_system);
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
// returns volume label
/////////////////////////////////////////////////////////////////////////////
char *SEQ_FILE_VolumeLabel(void)
{
  return vi.label;
}


/////////////////////////////////////////////////////////////////////////////
// opens a file for reading
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadOpen(PFILEINFO fileinfo, char *filepath)
{
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] Opening file '%s'\n", filepath);
#endif

  // enable caching
  // attention:  this feature has to be explicitely enabled, as it isn't reentrant
  // and requires to use the same buffer pointer whenever reading a file.
  DFS_CachingEnabledSet(1);

  if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_READ, sector, fileinfo) ) {
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

    if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_READ, sector, fileinfo) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] Still not able to open file - giving up!\n");
#endif
      return SEQ_FILE_ERR_OPEN_READ;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE] found '%s' of length %u\n", filepath, fileinfo->filelen);
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
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n", fileinfo->pointer, seq_file_dfs_errno);
#endif
      return SEQ_FILE_ERR_READ;
  }
  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[SEQ_FILE] Wrong successcount while reading from position 0x%08x (count: %d)\n", fileinfo->pointer, successcount);
#endif
    return SEQ_FILE_ERR_READCOUNT;
  }

  return 0; // no error
}

s32 SEQ_FILE_ReadLine(PFILEINFO fileinfo, u8 *buffer, u32 max_len)
{
  s32 status;
  u32 num_read = 0;

  while( fileinfo->pointer < fileinfo->filelen ) {
    status = SEQ_FILE_ReadBuffer(fileinfo, buffer, 1);

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
// Closes a file which has been read
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_ReadClose(PFILEINFO fileinfo)
{
  // close file
  DFS_Close(fileinfo);

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Opens a file for writing
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_WriteOpen(PFILEINFO fileinfo, char *filepath, u8 create)
{
  // reset filepos
  write_filepos = 0;

#if SEQ_FILE_WRITE_BUFFER_MALLOC
  // TK: write buffer now statically allocated for deterministic memory usage
  // malloc option still optional

  // try to allocate buffer for write sector
  write_buffer = (u8 *)pvPortMalloc(SECTOR_SIZE);
  if( write_buffer == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] failed to allocate %d bytes for write sector\n", SECTOR_SIZE);
#endif
    return SEQ_FILE_ERR_WRITE_MALLOC;
  }
#endif

  // it's better to disable caching here, since different sectors are accessed
  DFS_CachingEnabledSet(0);

  if( create ) {
    if( seq_file_dfs_errno = DFS_UnlinkFile(&vi, filepath, sector) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] couldn't delete '%s' (file not found...)\n", filepath);
#endif
      // continue .. perhaps it didn't exist
    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE] old '%s' has been deleted\n", filepath);
#endif
    }
  }
  
  if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_WRITE, sector, fileinfo) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE] error opening file '%s' for writing!\n", filepath);
#endif

#if SEQ_FILE_WRITE_BUFFER_MALLOC
    // free memory
    vPortFree(write_buffer);
#endif

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
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n", write_filepos, seq_file_dfs_errno);
#endif
      write_filepos = -1; // ensure that next writes will be skipped
      return SEQ_FILE_ERR_WRITE;
    }
    if( successcount != SECTOR_SIZE ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE] Wrong successcount while writing to position 0x%08x (count: %d)\n", write_filepos, successcount);
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
#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n", write_filepos-SECTOR_SIZE, seq_file_dfs_errno);
#endif

      status = -1; // write error
    } else {
      if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[SEQ_FILE] Wrong successcount while writing to position 0x%08x (count: %d)\n", write_filepos-SECTOR_SIZE, successcount);
#endif
	
	status = -2; // not all bytes written
      }
    }
  }

  // close file
  DFS_Close(fileinfo);

#if SEQ_FILE_WRITE_BUFFER_MALLOC
  // free memory
  vPortFree(write_buffer);
#endif

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


/////////////////////////////////////////////////////////////////////////////
// This function prints some useful SD card informations on the MIOS terminal
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_PrintSDCardInfos(void)
{
  int status = 0;
  DIRINFO di;
  DIRENT de;

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
  
  // try to mount file system
  u32 pstart, psize;
  u8  pactive, ptype;

  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
  if( pstart == 0xffffffff ) {
    DEBUG_MSG("ERROR: Cannot find first partition!\n");
    return SEQ_FILE_ERR_NO_PARTITION;
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
    return SEQ_FILE_ERR_NO_VOLUME;
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
    return SEQ_FILE_ERR_UNKNOWN_FS;
  }

  di.scratch = sector;
  if( DFS_OpenDir(&vi, "backup/7", &di) ) {
    DEBUG_MSG("ERROR: opening root directory - try mounting the partition again\n");
    return SEQ_FILE_ERR_OPEN_DIR;
  }

  DEBUG_MSG("Content of root directory:\n");
  u32 num_files = 0;
  while( !DFS_GetNext(&vi, &di, &de) ) {
    if( de.name[0] ) {
      u8 file_name[13];
      ++num_files;
      DFS_DirToCanonical(file_name, de.name);
      DEBUG_MSG("- %s\n", file_name);
    }
  }
  DEBUG_MSG("Found %u directory entries.\n", num_files);
  DEBUG_MSG("--------------------\n");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function copies a file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Copy(char *src_file, char *dst_file, u8 *write_buffer)
{
  s32 status = 0;
  FILEINFO fi_src, fi_dst;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_Copy] copy %s to %s\n", src_file, dst_file);
#endif

  // disable caching to avoid file inconsistencies while using different sector buffers!
  DFS_CachingEnabledSet(0);

  if( seq_file_dfs_errno = DFS_OpenFile(&vi, src_file, DFS_READ, sector, &fi_src) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] %s doesn't exist!\n", src_file);
#endif
    status = SEQ_FILE_ERR_COPY_NO_FILE;
  } else {
    // delete destination file if it already exists - ignore errors
    DFS_UnlinkFile(&vi, dst_file, sector);

    if( seq_file_dfs_errno = DFS_OpenFile(&vi, dst_file, DFS_WRITE, sector, &fi_dst) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE_Copy] wasn't able to create %s - exit!\n", dst_file);
#endif
      status = SEQ_FILE_ERR_COPY;
      DFS_Close(&fi_src);
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

    u32 successcount;
    u32 successcount_wr;
    u32 num_bytes = 0;
    do {
      if( seq_file_dfs_errno = DFS_ReadFile(&fi_src, sector, write_buffer, &successcount, SECTOR_SIZE) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE] Failed to read sector at position 0x%08x, status: %u\n", fi_src.pointer, seq_file_dfs_errno);
#endif
	successcount = 0;
	status = SEQ_FILE_ERR_READ;
      } else if( successcount && (seq_file_dfs_errno=DFS_WriteFile(&fi_dst, sector, write_buffer, &successcount_wr, successcount)) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE] Failed to write sector at position 0x%08x, status: %u\n", fi_dst.pointer, seq_file_dfs_errno);
#endif
	status = SEQ_FILE_ERR_WRITE;
      } else {
	num_bytes += successcount_wr;
	seq_file_copy_percentage = (u8)((100 * num_bytes) / fi_src.filelen);
      }
    } while( status == 0 && successcount > 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] Finished copy operation (%d bytes)!\n", num_bytes);
#endif

    DFS_Close(&fi_src);
    DFS_Close(&fi_dst);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// This function creates a backup of all MBSEQ files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_CreateBackup(void)
{
  s32 status = 0;
  DIRINFO di;
  DIRENT de;
  FILEINFO fi;

  if( !volume_available ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: volume doesn't exist!\n");
#endif
    return SEQ_FILE_ERR_NO_VOLUME;
  }

#if SEQ_FILE_WRITE_BUFFER_MALLOC
  // TK: write buffer now statically allocated for deterministic memory usage
  // malloc option still optional

  // try to allocate buffer for write sector
  write_buffer = (u8 *)pvPortMalloc(SECTOR_SIZE);
  if( write_buffer == NULL ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Copy] failed to allocate %d bytes for write sector\n", SECTOR_SIZE);
#endif
    return SEQ_FILE_ERR_WRITE_MALLOC;
  }
#endif

  // disable caching to avoid file inconsistencies while using different sector buffers!
  DFS_CachingEnabledSet(0);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_CreateBackup] Content of %s directory:\n", SEQ_FILE_BACKUP_PATH);
#endif

  // Note: we have to use a different buffer for directory scan, otherwise
  // DFS_OpenFile() will disturb it!
  di.scratch = write_buffer;
  if( DFS_OpenDir(&vi, SEQ_FILE_BACKUP_PATH, &di) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: opening %s directory - please create it!\n", SEQ_FILE_BACKUP_PATH);
#endif
    status = SEQ_FILE_ERR_NO_BACKUP_DIR;
  }

  int num_files = 0;
  while( status == 0 && !DFS_GetNext(&vi, &di, &de) ) {
    if( de.name[0] && de.name[0] != '.' ) {
      u8 file_name[13];
      ++num_files;
      DFS_DirToCanonical(file_name, de.name);

      char filepath[MAX_PATH];
      sprintf(filepath, "%s%s/MBSEQ_C.V4", SEQ_FILE_BACKUP_PATH, file_name);

      if( seq_file_dfs_errno = DFS_OpenFile(&vi, filepath, DFS_READ, sector, &fi) ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_CreateBackup] %s doesn't exist - take this dir!\n", filepath);
#endif

	// directory found - start to copy files
#define COPY_FILE_MACRO(name) if( status >= 0 ) { \
	  sprintf(filepath, "%s%s/%s", SEQ_FILE_BACKUP_PATH, file_name, name); \
	  seq_file_backup_notification = filepath; \
	  status = SEQ_FILE_Copy(name, filepath, write_buffer); \
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
	COPY_FILE_MACRO("MBSEQ_S.V4");
	COPY_FILE_MACRO("MBSEQ_C.V4"); // important: should be the last file to notify that backup is complete!

	// stop printing the special message
	seq_file_backup_notification = NULL;

	// stop directory scan
	if( status >= 0 )
	  status = 1; // we were successfull!
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_CreateBackup] %s already exists\n", filepath);
#endif
	DFS_Close(&fi);
      }
    }
  }

#if SEQ_FILE_WRITE_BUFFER_MALLOC
    // free memory
    vPortFree(write_buffer);
#endif

  // enable cache again
  DFS_CachingEnabledSet(1);

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
