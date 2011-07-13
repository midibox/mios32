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

#include "synth_file.h"
#include "synth_file_b.h"

#include "synth.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of bank file:
//    file_type[10]
//    synth_file_b_header_t
//    Patch 0: synth_file_b_patch_t
//    Patch 1: ...
//
// not defined as structure: 
// file_type[10] will contain "SYNTH_B" + 0 (zero-terminated string)
typedef struct {
  char name[20];      // bank name consists of 20 characters, no zero termination, patted with spaces
  u16  num_patches;   // number of patches per bank (usually 64)
  u32  patch_size;    // reserved size for each patch
} synth_file_b_header_t;  // 28 bytes

typedef struct {
  char name[20];      // patch name consists of 20 characters, no zero termination, patted with spaces
  u8   reserved1;     // reserved for future extensions
  u8   reserved2;     // reserved for future extensions
  u8   reserved3;     // reserved for future extensions
  u8   reserved4;     // reserved for future extensions
} synth_file_b_patch_t;  // 24 bytes



// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  synth_file_b_header_t header;

  synth_file_t file;      // file informations
} synth_file_b_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static synth_file_b_info_t synth_file_b_info[SYNTH_FILE_B_NUM_BANKS];

static u8 cached_patch_name[21];
static u8 cached_bank;
static u8 cached_patch;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_Init(u32 mode)
{
  // invalidate all bank infos
  SYNTH_FILE_B_UnloadAllBanks();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads all banks
// Called from SYNTH_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_LoadAllBanks(char *session)
{
  s32 status = 0;

  // load all banks
  u8 bank;
  for(bank=0; bank<SYNTH_FILE_B_NUM_BANKS; ++bank) {
    s32 error = SYNTH_FILE_B_Open(session, bank);
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] Tried to open bank #%d file, status: %d\n", bank+1, error);
#endif
#if 0
    if( error == -2 ) {
      error = SYNTH_FILE_B_Create(session, bank);
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SYNTH_FILE_B] Tried to create bank #%d file, status: %d\n", bank+1, error);
#endif
    }
#endif
    status |= error;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads all banks
// Called from SYNTH_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_UnloadAllBanks(void)
{
  // invalidate all bank infos
  u8 bank;
  for(bank=0; bank<SYNTH_FILE_B_NUM_BANKS; ++bank)
    synth_file_b_info[bank].valid = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of patches in bank
// Returns 0 if bank not valid
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_NumPatches(u8 bank)
{
  if( (bank < SYNTH_FILE_B_NUM_BANKS) && synth_file_b_info[bank].valid )
    return synth_file_b_info[bank].header.num_patches;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// create a complete bank file
// returns < 0 on errors (error codes are documented in synth_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_Create(char *session, u8 bank)
{
  if( bank >= SYNTH_FILE_B_NUM_BANKS )
    return SYNTH_FILE_B_ERR_INVALID_BANK;

  synth_file_b_info_t *info = &synth_file_b_info[bank];
  info->valid = 0; // set to invalid as long as we are not sure if file can be accessed

  char filepath[MAX_PATH];
#if 0
  sprintf(filepath, "%s/%s/SYNTH_B%d.V1", SYNTH_FILE_SESSION_PATH, session, bank+1);
#else
  // currently no sessions used
  sprintf(filepath, "/SYNTH_B%d.V1", bank+1);
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] Creating new bank file '%s'\n", filepath);
#endif

  s32 status = 0;
  if( (status=SYNTH_FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] Failed to create file, status: %d\n", status);
#endif
    return status;
  }

  // write synth_file_b_header
  const char file_type[10] = "SYNTHZZ_B";
  status |= SYNTH_FILE_WriteBuffer((u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= SYNTH_FILE_WriteBuffer((u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] writing '%s'...\n", bank_name);
#endif

  // number of patches
  info->header.num_patches = 64;
  status |= SYNTH_FILE_WriteHWord(info->header.num_patches);

  info->header.patch_size = SYNTH_NUM_PHRASES * (16*16 + 16); // currently hardcoded
  status |= SYNTH_FILE_WriteWord(info->header.patch_size);

  // close file
  status |= SYNTH_FILE_WriteClose();

  if( status >= 0 )
    // bank valid - caller should fill the patch slots with useful data now
    info->valid = 1;


#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] Bank file created with status %d\n", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in synth_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_Open(char *session, u8 bank)
{
  if( bank >= SYNTH_FILE_B_NUM_BANKS )
    return SYNTH_FILE_B_ERR_INVALID_BANK;

  synth_file_b_info_t *info = &synth_file_b_info[bank];

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
#if 0
  sprintf(filepath, "%s/%s/SYNTH_B%d.V1", SYNTH_FILE_SESSION_PATH, session, bank+1);
#else
  // currently no sessions used
  sprintf(filepath, "/SYNTH_B%d.V1", bank+1);
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] Open bank file '%s'\n", filepath);
#endif

  s32 status;
  if( (status=SYNTH_FILE_ReadOpen((synth_file_t*)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=SYNTH_FILE_ReadBuffer((u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] failed to read header, status: %d\n", status);
#endif
    return status;
  }

  if( strncmp(file_type, "SYNTHZZ_B", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    DEBUG_MSG("[SYNTH_FILE_B] wrong header type: %s\n", file_type);
#endif
    return SYNTH_FILE_B_ERR_FORMAT;
  }

  status |= SYNTH_FILE_ReadBuffer((u8 *)info->header.name, 20);
  status |= SYNTH_FILE_ReadHWord((u16 *)&info->header.num_patches);
  status |= SYNTH_FILE_ReadWord((u32 *)&info->header.patch_size);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] file access error while reading header, status: %d\n", status);
#endif
    return SYNTH_FILE_B_ERR_READ;
  }

  // close file (so that it can be re-opened)
  SYNTH_FILE_ReadClose((synth_file_t*)&info->file);

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] bank is valid! Number of Patches: %d, Patch Size: %d\n", info->header.num_patches, info->header.patch_size);
#endif

  // finally (re-)load cached patch name - status of this function doesn't matter
  char dummy[21];
  SYNTH_FILE_B_PatchPeekName(cached_bank, cached_patch, 1, dummy); // non-cached!

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a patch from bank into given group
// returns < 0 on errors (error codes are documented in synth_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_PatchRead(u8 bank, u8 patch, u8 target_group)
{
  if( bank >= SYNTH_FILE_B_NUM_BANKS )
    return SYNTH_FILE_B_ERR_INVALID_BANK;

  if( target_group >= SYNTH_NUM_GROUPS )
    return SYNTH_FILE_B_ERR_INVALID_GROUP;

  synth_file_b_info_t *info = &synth_file_b_info[bank];

  if( !info->valid )
    return SYNTH_FILE_B_ERR_NO_FILE;

  if( patch >= info->header.num_patches )
    return SYNTH_FILE_B_ERR_INVALID_PATCH;

  // re-open file
  if( SYNTH_FILE_ReadReOpen((synth_file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(synth_file_b_header_t) + patch * info->header.patch_size;
  if( (status=SYNTH_FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    SYNTH_FILE_ReadClose((synth_file_t*)&info->file);
    return SYNTH_FILE_B_ERR_READ;
  }

  u8 patchNameLen = 16; // failsave to avoid unwanted overwrite
  if( patchNameLen > SYNTH_PATCH_NAME_LEN )
    patchNameLen = SYNTH_PATCH_NAME_LEN;
  status |= SYNTH_FILE_ReadBuffer((u8 *)SYNTH_PatchNameGet(target_group), patchNameLen);
  u32 reservedName; // reserve 4 chars for longer name
  status |= SYNTH_FILE_ReadWord(&reservedName);

  u8 reserved1;
  status |= SYNTH_FILE_ReadByte(&reserved1);
  u8 reserved2;
  status |= SYNTH_FILE_ReadByte(&reserved2);
  u8 reserved3;
  status |= SYNTH_FILE_ReadByte(&reserved3);
  u8 reserved4;
  status |= SYNTH_FILE_ReadByte(&reserved4);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] read patch B%d:P%d '%s'\n", bank+1, patch, SYNTH_PatchNameGet(target_group));
#endif

  int phrase;
  for(phrase=0; phrase<SYNTH_NUM_PHRASES; ++phrase) {
    int ix;
    for(ix=0; ix<SYNTH_PHRASE_MAX_LENGTH; ++ix) {
      int par;
      for(par=0; par<16; ++par) {
	u8 value;
	status |= SYNTH_FILE_ReadByte(&value);
	SYNTH_PhonemeParSet(phrase, ix, par, value);
      }
    }

    int par;
    for(par=0; par<16; ++par) {
      u8 value;
      status |= SYNTH_FILE_ReadByte(&value);
      SYNTH_PhraseParSet(phrase, par, value);
    }
  }

  // close file (so that it can be re-opened)
  SYNTH_FILE_ReadClose((synth_file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] error while reading file, status: %d\n", status);
#endif
    return SYNTH_FILE_B_ERR_READ;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a patch of a given group into bank
// returns < 0 on errors (error codes are documented in synth_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_PatchWrite(char *session, u8 bank, u8 patch, u8 source_group, u8 rename_if_empty_name)
{
  if( bank >= SYNTH_FILE_B_NUM_BANKS )
    return SYNTH_FILE_B_ERR_INVALID_BANK;

  if( source_group >= SYNTH_NUM_GROUPS )
    return SYNTH_FILE_B_ERR_INVALID_GROUP;

  synth_file_b_info_t *info = &synth_file_b_info[bank];

  if( !info->valid )
    return SYNTH_FILE_B_ERR_FORMAT;

  if( patch >= info->header.num_patches )
    return SYNTH_FILE_B_ERR_INVALID_PATCH;

  char filepath[MAX_PATH];
#if 0
  sprintf(filepath, "%s/%s/SYNTH_B%d.V1", SYNTH_FILE_SESSION_PATH, session, bank+1);
#else
  // currently no sessions used
  sprintf(filepath, "/SYNTH_B%d.V1", bank+1);
#endif

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] Open bank file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SYNTH_FILE_WriteOpen(filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] Failed to open file, status: %d\n", status);
#endif
    SYNTH_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(synth_file_b_header_t) + patch * info->header.patch_size;
  if( (status=SYNTH_FILE_WriteSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    SYNTH_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // rename patch if name is empty
  if( rename_if_empty_name ) {
    int i;
    u8 found_char = 0;
    u8 *label = (u8 *)SYNTH_PatchNameGet(source_group);
    for(i=0; i<SYNTH_PATCH_NAME_LEN; ++i)
      if( label[i] != ' ' ) {
	found_char = 1;
	break;
      }

    if( !found_char )
      memcpy(label, "Unnamed        ", 15);
  }

  // write patch name w/o zero terminator
  status |= SYNTH_FILE_WriteBuffer((u8 *)SYNTH_PatchNameGet(source_group), 16);
  u32 reservedName; // reserve 4 chars for longer name
  status |= SYNTH_FILE_ReadWord(&reservedName);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SYNTH_FILE_B] writing patch '%s'...\n", SYNTH_PatchNameGet(source_group));
#endif

  // reserved
  status |= SYNTH_FILE_WriteByte(0x00);
  status |= SYNTH_FILE_WriteByte(0x00);
  status |= SYNTH_FILE_WriteByte(0x00);
  status |= SYNTH_FILE_WriteByte(0x00);

  // writing phrases
  int phrase;
  for(phrase=0; phrase<SYNTH_NUM_PHRASES; ++phrase) {
    int ix;
    for(ix=0; ix<SYNTH_PHRASE_MAX_LENGTH; ++ix) {
      int par;
      for(par=0; par<16; ++par) {
	status |= SYNTH_FILE_WriteByte(SYNTH_PhonemeParGet(phrase, ix, par));
      }
    }

    int par;
    for(par=0; par<16; ++par) {
      status |= SYNTH_FILE_WriteByte(SYNTH_PhraseParGet(phrase, par));
    }
  }

  // close file
  status |= SYNTH_FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SYNTH_FILE_B] Patch written with status %d\n", status);
#endif

  return (status < 0) ? SYNTH_FILE_B_ERR_WRITE : 0;
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
// returns < 0 on errors (error codes are documented in synth_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FILE_B_PatchPeekName(u8 bank, u8 patch, u8 non_cached, char *patch_name)
{
  if( !non_cached && cached_bank == bank && cached_patch == patch ) {
    // name is in cache
    memcpy(patch_name, cached_patch_name, 21);
    return 0; // no error
  }

  cached_bank = bank;
  cached_patch = patch;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SYNTH_FILE_B] Loading Patch Name for %d:%d\n", bank, patch);
#endif

  // initial patch name
  memcpy(cached_patch_name, "-----<Disk Error>   ", 21);

  if( bank >= SYNTH_FILE_B_NUM_BANKS )
    return SYNTH_FILE_B_ERR_INVALID_BANK;

  synth_file_b_info_t *info = &synth_file_b_info[bank];

  if( !info->valid )
    return SYNTH_FILE_B_ERR_NO_FILE;

  if( patch >= info->header.num_patches )
    return SYNTH_FILE_B_ERR_INVALID_PATCH;

  // re-open file
  if( SYNTH_FILE_ReadReOpen((synth_file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(synth_file_b_header_t) + patch * info->header.patch_size;
  if( (status=SYNTH_FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SYNTH_FILE_B] failed to change patch offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    SYNTH_FILE_ReadClose((synth_file_t*)&info->file);
    return SYNTH_FILE_B_ERR_READ;
  }

  // read name
  status |= SYNTH_FILE_ReadBuffer((u8 *)cached_patch_name, 20);
  cached_patch_name[20] = 0;

  // close file (so that it can be re-opened)
  SYNTH_FILE_ReadClose((synth_file_t*)&info->file);

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
  DEBUG_MSG("[SYNTH_FILE_B] Loading Patch Name for %d:%d successfull\n", bank, patch);
#endif

  return 0; // no error
}
