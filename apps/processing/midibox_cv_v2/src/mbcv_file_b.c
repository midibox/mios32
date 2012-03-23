// $Id$
/*
 * Bank access functions
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

#include <string.h>

#include "file.h"
#include "mbcv_file.h"
#include "mbcv_file_b.h"

#include "mbcv_patch.h"



/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be 1 by default to ensure that error messages are print
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MBCV_FILES_PATH "/"
//#define MBCV_FILES_PATH "/MySongs/"


// defined locally to avoid usage of C++ constants
// TODO: should be overworked
#define CV_PATCH_CHANNELS 8
#define CV_PATCH_SIZE     0x400 // in halfwords!
#define CV_NUM_PATCHES    128



/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of bank file:
//    file_type[10]
//    mbcv_file_b_header_t
//    Patch 0: mbcv_file_b_patch_t
//               Channel 0: mbcv_file_b_channel_t
//               Channel 1: ...
//    Patch 1: ...
//
// Size for 128 patches
// each consists of 0x400 halfwords = 0x800 bytes (2k)
// 10 + 24 + 128 * (24 + 8 * 2048)
// 10 + 24 + 128 * 16408
// 10 + 24 + 2100224
// -> 2100258 bytes (good that SD cards are so cheap today ;)

// not defined as structure: 
// file_type[10] will contain "MBCV2_B" + 0 (zero-terminated string)
typedef struct {
  char name[20];      // bank name consists of 20 characters, no zero termination, patted with spaces
  u16  num_patches;   // number of patches per bank (usually 128)
  u16  patch_size;    // reserved size for each patch
} mbcv_file_b_header_t;  // 24 bytes

typedef struct {
  char name[20];      // patch name consists of 20 characters, no zero termination, patted with spaces
  u8   num_channels;  // number of channels in patch (usually 8)
  u8   reserved1;     // reserved for future extensions
  u8   reserved2;     // reserved for future extensions
  u8   reserved3;     // reserved for future extensions
} mbcv_file_b_patch_t;  // 24 bytes


// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  mbcv_file_b_header_t header;

  file_t file;      // file informations
} mbcv_file_b_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static mbcv_file_b_info_t mbcv_file_b_info[MBCV_FILE_B_NUM_BANKS];

static u8 cached_patch_name[21];
static u8 cached_bank;
static u8 cached_patch;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_Init(u32 mode)
{
  // invalidate all bank infos
  MBCV_FILE_B_UnloadAllBanks();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads all banks
// Called from MBCV_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_LoadAllBanks(void)
{
  s32 status = 0;

  // load all banks
  u8 bank;
  for(bank=0; bank<MBCV_FILE_B_NUM_BANKS; ++bank) {
    s32 error = MBCV_FILE_B_Open(bank);
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBCV_FILE_B] Tried to open bank %c file, status: %d\n", 'A'+bank, error);
#endif
#if 0
    if( error == -2 ) {
      error = MBCV_FILE_B_Create(bank);
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[MBCV_FILE_B] Tried to create bank %c file, status: %d\n", 'A'+bank, error);
#endif
    }
#endif
    status |= error;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads all banks
// Called from MBCV_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_UnloadAllBanks(void)
{
  // invalidate all bank infos
  u8 bank;
  for(bank=0; bank<MBCV_FILE_B_NUM_BANKS; ++bank)
    mbcv_file_b_info[bank].valid = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of patches in bank
// Returns 0 if bank not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_NumPatches(u8 bank)
{
  if( (bank < MBCV_FILE_B_NUM_BANKS) && mbcv_file_b_info[bank].valid )
    return mbcv_file_b_info[bank].header.num_patches;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// create a complete bank file
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_Create(u8 bank)
{
  if( bank >= MBCV_FILE_B_NUM_BANKS )
    return MBCV_FILE_B_ERR_INVALID_BANK;

  mbcv_file_b_info_t *info = &mbcv_file_b_info[bank];
  info->valid = 0; // set to invalid as long as we are not sure if file can be accessed

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBCV_B%d.V2", MBCV_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Creating new bank file '%s'\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] Failed to create file, status: %d\n", status);
#endif
    return status;
  }

  // write mbcv_file_b_header
  const char file_type[10] = "MBCV2_B";
  status |= FILE_WriteBuffer((u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= FILE_WriteBuffer((u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] writing '%s'...\n", bank_name);
#endif

  // number of patches and size
  info->header.num_patches = CV_NUM_PATCHES;
  info->header.patch_size = sizeof(mbcv_file_b_patch_t) + CV_PATCH_CHANNELS * 2 * CV_PATCH_SIZE;
  status |= FILE_WriteHWord(info->header.num_patches);
  status |= FILE_WriteHWord(info->header.patch_size);

  // close file
  status |= FILE_WriteClose();

  if( status >= 0 )
    // bank valid - caller should fill the patch slots with useful data now
    info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Bank file created with status %d\n", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_Open(u8 bank)
{
  if( bank >= MBCV_FILE_B_NUM_BANKS )
    return MBCV_FILE_B_ERR_INVALID_BANK;

  mbcv_file_b_info_t *info = &mbcv_file_b_info[bank];

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBCV_B%d.V2", MBCV_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Open bank file '%s'\n", filepath);
#endif

  s32 status;
  if( (status=FILE_ReadOpen((file_t*)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=FILE_ReadBuffer((u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] failed to read header, status: %d\n", status);
#endif
    return status;
  }

  if( strncmp(file_type, "MBCV2_B", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    DEBUG_MSG("[MBCV_FILE_B] wrong header type: %s\n", file_type);
#endif
    return MBCV_FILE_B_ERR_FORMAT;
  }

  status |= FILE_ReadBuffer((u8 *)info->header.name, 20);
  status |= FILE_ReadHWord((u16 *)&info->header.num_patches);
  status |= FILE_ReadHWord((u16 *)&info->header.patch_size);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] file access error while reading header, status: %d\n", status);
#endif
    return MBCV_FILE_B_ERR_READ;
  }

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] bank is valid! Number of Patches: %d, Patch Size: %d\n", info->header.num_patches, info->header.patch_size);
#endif

  // finally (re-)load cached patch name - status of this function doesn't matter
  char dummy[21];
  MBCV_FILE_B_PatchPeekName(cached_bank, cached_patch, 1, dummy); // non-cached!

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a patch from bank
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_PatchRead(u8 bank, u8 patch)
{
  if( bank >= MBCV_FILE_B_NUM_BANKS )
    return MBCV_FILE_B_ERR_INVALID_BANK;

  mbcv_file_b_info_t *info = &mbcv_file_b_info[bank];

  if( !info->valid )
    return MBCV_FILE_B_ERR_NO_FILE;

  if( patch >= info->header.num_patches )
    return MBCV_FILE_B_ERR_INVALID_PATCH;

  // re-open file
  if( FILE_ReadReOpen((file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(mbcv_file_b_header_t) + patch * info->header.patch_size;
  if( (status=FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    FILE_ReadClose((file_t*)&info->file);
    return MBCV_FILE_B_ERR_READ;
  }

  status |= FILE_ReadBuffer((u8 *)mbcv_patch_name, 20);
  mbcv_patch_name[20] = 0;

  u8 num_channels = 0;
  status |= FILE_ReadByte(&num_channels);

  u8 reserved = 0;
  status |= FILE_ReadByte(&reserved);
  status |= FILE_ReadByte(&reserved);
  status |= FILE_ReadByte(&reserved);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] read patch %c%03d '%s', %d channels\n", 'A'+bank, patch, mbcv_patch_name, num_channels);
#endif

  // reduce number of channels if required
  if( num_channels > CV_PATCH_CHANNELS )
    num_channels = CV_PATCH_CHANNELS;

  if( status >= 0 ) {
    u8 cv;
    for(cv=0; cv<CV_PATCH_CHANNELS; ++cv) {
      status |= FILE_ReadBuffer((u8 *)MBCV_PATCH_CopyBufferGet(), 2*CV_PATCH_SIZE);

      // check status before pasting into CV channel
      if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBCV_FILE_B] read channel #%d failed due to file access error, status: %d\n", cv+1, status);
#endif
	break;
      }

      MBCV_PATCH_Paste(cv, (u8 *)MBCV_PATCH_CopyBufferGet());
    }
  }

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] error while reading file, status: %d\n", status);
#endif
    return MBCV_FILE_B_ERR_READ;
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] read finished successfully!\n");
#endif
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a patch into bank
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_PatchWrite(u8 bank, u8 patch, u8 rename_if_empty_name)
{
  if( bank >= MBCV_FILE_B_NUM_BANKS )
    return MBCV_FILE_B_ERR_INVALID_BANK;

  mbcv_file_b_info_t *info = &mbcv_file_b_info[bank];

  if( !info->valid )
    return MBCV_FILE_B_ERR_FORMAT;

  if( patch >= info->header.num_patches )
    return MBCV_FILE_B_ERR_INVALID_PATCH;


  // TODO: before writing into patch slot, we should check if it already exists, and then
  // compare layer parameters with given constraints available in following defines/variables:

  // ok, we should at least check, if the resulting size is within the given range
  u16 expected_patch_size = sizeof(mbcv_file_b_patch_t) + CV_PATCH_CHANNELS * 2 * CV_PATCH_SIZE;

  if( expected_patch_size > info->header.patch_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] Resulting patch is too large for slot in bank (is: %d, max: %d)\n", 
	   expected_patch_size, info->header.patch_size);
    return MBCV_FILE_B_ERR_P_TOO_LARGE;
#endif
  }

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBCV_B%d.V2", MBCV_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Open bank file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] Failed to open file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(mbcv_file_b_header_t) + patch * info->header.patch_size;
  if( (status=FILE_WriteSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // rename patch if name is empty
  if( rename_if_empty_name ) {
    int i;
    u8 found_char = 0;
    u8 *label = (u8 *)&mbcv_patch_name[5];
    for(i=0; i<15; ++i)
      if( label[i] != ' ' ) {
	found_char = 1;
	break;
      }

    if( !found_char )
      memcpy(label, "Unnamed        ", 15);
  }

  // write patch name w/o zero terminator
  status |= FILE_WriteBuffer((u8 *)mbcv_patch_name, 20);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] writing patch %c%03d '%s'...\n", 'A'+bank, patch, mbcv_patch_name);
#endif

  // header information
  u8 num_channels = CV_PATCH_CHANNELS;
  status |= FILE_WriteByte(num_channels);

  u8 reserved = 0;
  status |= FILE_WriteByte(reserved);
  status |= FILE_WriteByte(reserved);
  status |= FILE_WriteByte(reserved);


  // write patches
  u8 cv;
  for(cv=0; cv<num_channels; ++cv) {
    MBCV_PATCH_Copy(cv, (u8 *)MBCV_PATCH_CopyBufferGet());
    status |= FILE_WriteBuffer((u8 *)MBCV_PATCH_CopyBufferGet(), 2*CV_PATCH_SIZE);
  }


  // fill remaining bytes with zero if required
  while( expected_patch_size < info->header.patch_size ) {
    status |= FILE_WriteByte(0x00);
    ++expected_patch_size;
  }

  // close file
  status |= FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Patch written with status %d\n", status);
#endif

  return (status < 0) ? MBCV_FILE_B_ERR_WRITE : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns a patch name from disk w/o overwriting patches in RAM
//
// used in SAVE menu to display the patch name which will be overwritten
// 
// function can be called frequently w/o performance loss, as the name
// of bank/patch will be cached.
// non_cached=1 forces an update regardless of bank/patch number
//
// *name will contain 20 characters + 0 terminator regardless of status
//
// returns < 0 on errors (error codes are documented in mbcv_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBCV_FILE_B_PatchPeekName(u8 bank, u8 patch, u8 non_cached, char *patch_name)
{
  if( !non_cached && cached_bank == bank && cached_patch == patch ) {
    // name is in cache
    memcpy(patch_name, cached_patch_name, 21);
    return 0; // no error
  }

  cached_bank = bank;
  cached_patch = patch;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Loading Patch Name for %c%03d\n", 'A'+bank, patch);
#endif

  // initial patch name
  memcpy(cached_patch_name, "-----<Disk Error>   ", 21);

  if( bank >= MBCV_FILE_B_NUM_BANKS )
    return MBCV_FILE_B_ERR_INVALID_BANK;

  mbcv_file_b_info_t *info = &mbcv_file_b_info[bank];

  if( !info->valid )
    return MBCV_FILE_B_ERR_NO_FILE;

  if( patch >= info->header.num_patches )
    return MBCV_FILE_B_ERR_INVALID_PATCH;

  // re-open file
  if( FILE_ReadReOpen((file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(mbcv_file_b_header_t) + patch * info->header.patch_size;
  if( (status=FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBCV_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    FILE_ReadClose((file_t*)&info->file);
    return MBCV_FILE_B_ERR_READ;
  }

  // read name
  status |= FILE_ReadBuffer((u8 *)cached_patch_name, 20);
  cached_patch_name[20] = 0;

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  // fill category with "-----" if it is empty
  int i;
  u8 found_char = 0;
  for(i=0; i<5; ++i)
    if( cached_patch_name[i] != ' ' ) {
      found_char = 1;
      break;
    }
  if( !found_char )
    memcpy(&cached_patch_name[0], "-----", 5);


  // fill label with "<empty>" if it is empty
  found_char = 0;
  for(i=5; i<20; ++i)
    if( cached_patch_name[i] != ' ' ) {
      found_char = 1;
      break;
    }
  if( !found_char )
    memcpy(&cached_patch_name[5], "<empty>        ", 15);

  
  // copy into return variable
  memcpy(patch_name, cached_patch_name, 21);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBCV_FILE_B] Loading Patch Name for %c%03d successfull\n", 'A'+bank, patch);
#endif

  return 0; // no error
}
