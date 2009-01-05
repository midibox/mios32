// $Id$
/*
 * MIDI File Access Routines
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
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <string.h>

#include "mid_file.h"

#include <dosfs.h>


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
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the .mid files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MID_FILES_PATH ""
//#define MID_FILES_PATH "MyMidi/"



/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 MID_FILE_mount_fs();


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the file position and length
static u32 midifile_pos;
static u32 midifile_len;
static u8  midifile_name[13];

// DOS FS variables
static u8 sector[SECTOR_SIZE];
static VOLINFO vi;
static DIRINFO di;
static DIRENT de;
static FILEINFO fi;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_Init(u32 mode)
{
  // initial midifile name and size
  strcpy(midifile_name, "waiting...");
  midifile_len = 0;

  // init SDCard access
  s32 error = MIOS32_SDCARD_Init(0);
#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[MID_FILE] SD Card initialized, status: %d\n\r", error);
#endif

  if( error >= 0 ) {
    error = MIOS32_SDCARD_PowerOn();
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] SD Card power on sequence, status: %d\n\r", error);
#endif
  }

  if( error >= 0 ) {
    error = MID_FILE_mount_fs();
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] Tried to mount file system, status: %d\n\r", error);
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the filename of the .mid file
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_NameGet(void)
{
  return midifile_name;
}


/////////////////////////////////////////////////////////////////////////////
// Mount the file system
/////////////////////////////////////////////////////////////////////////////
static s32 MID_FILE_mount_fs(void)
{
  u32 pstart, psize;
  u8  pactive, ptype;

  // Obtain pointer to first partition on first (only) unit
  pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
  if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] Cannot find first partition - reconnect SD Card\n\r");
#endif

    s32 error = MIOS32_SDCARD_PowerOn();
    if( error < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[MID_FILE] Failed - no access to SD Card\n\r");
#endif
      return -1;
    }

    pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);
    if( pstart == 0xffffffff ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[MID_FILE] SD Card available, but still cannot find first partition - giving up!\n\r");
#endif
      return -1;
    }
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[MID_FILE] Partition 0 start sector 0x%-08.8lX active %-02.2hX type %-02.2hX size %-08.8lX\n\r", pstart, pactive, ptype, psize);
#endif

  if (DFS_GetVolInfo(0, sector, pstart, &vi)) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] Error getting volume information\n\r");
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[MID_FILE] Volume label '%s'\n\r", vi.label);
  printf("[MID_FILE] %d sector/s per cluster, %d reserved sector/s, volume total %d sectors.\n\r", vi.secperclus, vi.reservedsecs, vi.numsecs);
  printf("[MID_FILE] %d sectors per FAT, first FAT at sector #%d, root dir at #%d.\n\r",vi.secperfat,vi.fat1,vi.rootdir);
  printf("[MID_FILE] (For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n\r");
  printf("[MID_FILE] %d root dir entries, data area commences at sector #%d.\n\r",vi.rootentries,vi.dataarea);
  printf("[MID_FILE] %d clusters (%d bytes) in data area, filesystem IDd as ", vi.numclusters, vi.numclusters * vi.secperclus * SECTOR_SIZE);
  if (vi.filesystem == FAT12)
    printf("FAT12.\n\r");
  else if (vi.filesystem == FAT16)
    printf("FAT16.\n\r");
  else if (vi.filesystem == FAT32)
    printf("FAT32.\n\r");
  else {
    printf("[MID_FILE] [unknown]\n\r");
  }
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
// Returns the .mid file next to the given filename
// if filename == NULL, the first .mid file will be returned
// if returned filename == NULL, no .mid file has been found
// dest must point to a 13-byte buffer
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_FindNext(char *filename)
{
#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[MID_FILE] Opening directory '%s'\n\r", MID_FILES_PATH);
#endif

  di.scratch = sector;
  if( DFS_OpenDir(&vi, MID_FILES_PATH, &di) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] Error opening directory - try mounting the partition again\n\r");
#endif

    s32 error;
    if( (error = MID_FILE_mount_fs()) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[MID_FILE] mounting failed with status: %d\n\r", error);
#endif
      strcpy(midifile_name, "no SD-Card");
      return NULL; // directory not found
    }

    if( DFS_OpenDir(&vi, MID_FILES_PATH, &di) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[MID_FILE] Error opening directory again - giving up!\n\r");
#endif
      strcpy(midifile_name, "no direct.");
      return NULL; // directory not found
    }
  }

  u8 take_next;
  u8 *search_file[13];
  if( filename == NULL ) {
    take_next = 1;
  } else {
    take_next = 0;
    DFS_CanonicalToDir(search_file, filename);
  }

  while( !DFS_GetNext(&vi, &di, &de) ) {
    if( de.name[0] ) {
      DFS_DirToCanonical(midifile_name, de.name);
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[MID_FILE] file: '%s'\n\r", midifile_name);
#endif
      if( strncasecmp(&de.name[8], "mid", 3) == 0 ) {
	if( take_next ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	  printf("[MID_FILE] return it as next file\n\r");
#endif
	  return midifile_name;
	} else if( strncasecmp(&de.name[0], (char *)search_file, 8) == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	  printf("[MID_FILE] found file: '%s', searching for next\n\r", midifile_name);
#endif
	  take_next = 1;
	}
      }
    }
  }

  midifile_name[0] = 0;
  return NULL; // file not found
}


/////////////////////////////////////////////////////////////////////////////
// Open a .mid file with given filename
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_open(char *filename)
{
  char filepath[MAX_PATH];
  strncpy(filepath, MID_FILES_PATH, MAX_PATH);
  strncat(filepath, filename, MAX_PATH);
  
  if( DFS_OpenFile(&vi, filepath, DFS_READ, sector, &fi)) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] error opening file '%s'!\n\r", filepath);
#endif
    strcpy(midifile_name, "file error");
    midifile_len = 0;
    return -1;
  }

  // got it
  midifile_pos = 0;
  midifile_len = fi.filelen;

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[MID_FILE] opened '%s' of length %u\n\r", filepath, midifile_len);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 MID_FILE_read(void *buffer, u32 len)
{
  u32 successcount;

  DFS_ReadFile(&fi, sector, buffer, &successcount, len);

  if( successcount != len ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[MID_FILE] unexpected read error - DFS cannot access offset %u (len: %u, got: %u)\n\r", midifile_pos, len, successcount);
#endif
  }
  
  midifile_pos += len; // no error, just failsafe: hold the reference file position, regardless of successcount

  return successcount;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_eof(void)
{
  if( midifile_pos >= midifile_len )
    return 1; // end of file reached

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_seek(u32 pos)
{
  midifile_pos = pos;
  if( midifile_pos >= midifile_len )
    return -1; // end of file reached

  DFS_Seek(&fi, pos, sector);

  return 0;
}
