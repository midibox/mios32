// $Id$
/*
 * Mixer map access functions
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

#include "seq_file.h"
#include "seq_file_m.h"

#include "seq_mixer.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of mixer map file:
//    file_type[10]
//    seq_file_m_header_t
//    Map 0: seq_file_m_map_header_t
//           mixer map
//    Map 1: ...
//
// Size for 128 maps
// each consists of 16 channels and 16 parameters (format is flexible enough to change these numbers)
// 10 + 24 + 128 * (22 + 16 * 16)
// 10 + 24 + 128 * (278)
// 10 + 24 + 35584
// -> 35618 bytes

// not defined as structure: 
// file_type[10] will contain "MBSEQV4_M" + 0 (zero-terminated string)
typedef struct {
  char name[20];      // mixer bank name consists of 20 characters, no zero termination, patted with spaces
  u16  num_maps;      // number of maps per bank (usually 128)
  u16  map_size;      // reserved size for each map
} seq_file_m_header_t;  // 24 bytes

typedef struct {
  char name[20];      // map name consists of 20 characters, no zero termination, patted with spaces
  u8   num_chn;       // number of channels in map (usually 16)
  u8   num_par;       // number of parameters per channel (usually 16)
} seq_file_m_map_header_t; // 22 bytes



// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  seq_file_m_header_t header;

  seq_file_t file;      // file informations
} seq_file_m_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_m_info_t seq_file_m_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_Init(u32 mode)
{
  // invalidate bank info
  SEQ_FILE_M_UnloadAllBanks();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_LoadAllBanks(char *session)
{
  // load bank
  s32 error;
  error = SEQ_FILE_M_Open(session);
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Tried to open bank file, status: %d\n", error);
#endif
#if 0
  if( error == -2 ) {
    error = SEQ_FILE_M_Create(session);
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] Tried to create bank file, status: %d\n", error);
#endif
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_UnloadAllBanks(void)
{
  seq_file_m_info.valid = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of maps in bank
// Returns 0 if bank not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_NumMaps(void)
{
  return seq_file_m_info.header.num_maps;
}


/////////////////////////////////////////////////////////////////////////////
// create a complete bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_Create(char *session)
{
  seq_file_m_info_t *info = &seq_file_m_info;
  info->valid = 0; // set to invalid as long as we are not sure if file can be accessed

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_M.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Creating new bank file '%s'\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] Failed to create file, status: %d\n", status);
#endif
    return status;
  }

  // write seq_file_m_header
  const char file_type[10] = "MBSEQV4_M";
  status |= SEQ_FILE_WriteBuffer((u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= SEQ_FILE_WriteBuffer((u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] writing '%s'...\n", bank_name);
#endif

  // number of maps
  info->header.num_maps = SEQ_MIXER_NUM;
  status |= SEQ_FILE_WriteHWord(info->header.num_maps);

  // write predefined map size
  u8 num_chn = 16;
  u8 num_par = 16;

  info->header.map_size = sizeof(seq_file_m_map_header_t) + num_chn * num_par;
  status |= SEQ_FILE_WriteHWord(info->header.map_size);


  // not required anymore with FatFs (was required with DOSFS)
#if 0
  // create empty map slots
  u32 map;
  for(map=0; map<info->header.num_maps; ++map) {
    u32 pos;
    for(pos=0; pos<info->header.map_size; ++pos)
      status |= SEQ_FILE_WriteByte(0x00);
  }
#endif

  // close file
  status |= SEQ_FILE_WriteClose();

  if( status >= 0 )
    // bank valid - caller should fill the map slots with useful data now
    info->valid = 1;


#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Bank file created with status %d\n", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_Open(char *session)
{
  seq_file_m_info_t *info = &seq_file_m_info;

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_M.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Open bank file '%s'\n", filepath);
#endif

  s32 status;
  if( (status=SEQ_FILE_ReadOpen((seq_file_t*)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=SEQ_FILE_ReadBuffer((u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] failed to read header, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);
    return status;
  }

  if( strncmp(file_type, "MBSEQV4_M", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    DEBUG_MSG("[SEQ_FILE_M] wrong header type: %s\n", file_type);
#endif
    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);
    return SEQ_FILE_M_ERR_FORMAT;
  }

  status |= SEQ_FILE_ReadBuffer((u8 *)info->header.name, 20);
  status |= SEQ_FILE_ReadHWord((u16 *)&info->header.num_maps);
  status |= SEQ_FILE_ReadHWord((u16 *)&info->header.map_size);

  // close file (so that it can be re-opened)
  SEQ_FILE_ReadClose((seq_file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] file access error while reading header, status: %d\n", status);
#endif
    return SEQ_FILE_M_ERR_READ;
  }

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] bank is valid! Number of Maps: %d, Map Size: %d\n", info->header.num_maps, info->header.map_size);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a map from bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_MapRead(u8 map)
{
  seq_file_m_info_t *info = &seq_file_m_info;

  if( !info->valid )
    return SEQ_FILE_M_ERR_NO_FILE;

  if( map >= info->header.num_maps )
    return SEQ_FILE_M_ERR_INVALID_MAP;

  // re-open file
  if( SEQ_FILE_ReadReOpen((seq_file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(seq_file_m_header_t) + map * info->header.map_size;
  if( (status=SEQ_FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] failed to change map offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);
    return SEQ_FILE_M_ERR_READ;
  }

  status |= SEQ_FILE_ReadBuffer((u8 *)seq_mixer_map_name, 20);
  seq_mixer_map_name[20] = 0;

  u8 num_chn;
  status |= SEQ_FILE_ReadByte(&num_chn);
  u8 num_par;
  status |= SEQ_FILE_ReadByte(&num_par);


#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] read map M%d '%s', %d channels, %d parameters\n", map, seq_mixer_map_name, num_chn, num_par);
#endif

  // reduce number of channels if required
  if( num_chn > SEQ_MIXER_NUM_CHANNELS )
    num_chn = SEQ_MIXER_NUM_CHANNELS;

  // reduce number of parameters if required
  if( num_par > SEQ_MIXER_NUM_PARAMETERS )
    num_par = SEQ_MIXER_NUM_PARAMETERS;

  u8 chn;
  for(chn=0; chn<num_chn; ++chn) {
    u8 par;
    for(par=0; par<num_par; ++par) {
      u8 value;
      status |= SEQ_FILE_ReadByte(&value);
      SEQ_MIXER_Set(chn, par, value);
    }
  }

  // close file (so that it can be re-opened)
  SEQ_FILE_ReadClose((seq_file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] error while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_M_ERR_READ;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a map into bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_M_MapWrite(char *session, u8 map, u8 rename_if_empty_name)
{
  seq_file_m_info_t *info = &seq_file_m_info;

  if( !info->valid )
    return SEQ_FILE_M_ERR_FORMAT;

  if( map >= info->header.num_maps )
    return SEQ_FILE_M_ERR_INVALID_MAP;


  // TODO: before writing into map slot, we should check if it already exists, and then
  // compare parameters with given constraints available in following defines/variables:
  u8 num_chn = SEQ_MIXER_NUM_CHANNELS;
  u8 num_par = SEQ_MIXER_NUM_PARAMETERS;

  // ok, we should at least check, if the resulting size is within the given range
  u16 expected_map_size = sizeof(seq_file_m_map_header_t) + num_chn * num_par;
  if( expected_map_size > info->header.map_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] Resulting map is too large for slot in bank (is: %d, max: %d)\n", 
	   expected_map_size, info->header.map_size);
    return SEQ_FILE_M_ERR_M_TOO_LARGE;
#endif
  }

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_M.V4", SEQ_FILE_SESSION_PATH, session);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Open bank file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] Failed to open file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(seq_file_m_header_t) + map * info->header.map_size;
  if( (status=SEQ_FILE_WriteSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_M] failed to change map offset in file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // rename map if name is empty
  if( rename_if_empty_name ) {
    int i;
    u8 found_char = 0;
    for(i=0; i<20; ++i)
      if( seq_mixer_map_name[i] != ' ' ) {
	found_char = 1;
	break;
      }

    if( !found_char )
      memcpy(seq_mixer_map_name, "Unnamed             ", 20);
  }

  // write map name w/o zero terminator
  status |= SEQ_FILE_WriteBuffer((u8 *)seq_mixer_map_name, 20);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_M] writing map #%d '%s'...\n", map, seq_mixer_map_name);
#endif

  // write number of channels
  status |= SEQ_FILE_WriteByte(num_chn);

  // write number of parameters
  status |= SEQ_FILE_WriteByte(num_par);

  // writing parameters
  u8 chn;
  for(chn=0; chn<num_chn; ++chn) {
    u8 par;
    for(par=0; par<num_par; ++par)
      status |= SEQ_FILE_WriteByte(SEQ_MIXER_Get(chn, par));
  }

  // fill remaining bytes with zero if required
  if( expected_map_size < info->header.map_size ) {
    int i;
    for(i=expected_map_size; i<info->header.map_size; ++i)
      status |= SEQ_FILE_WriteByte(0x00);
  }

  // close file
  status |= SEQ_FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_M] Map written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_M_ERR_WRITE : 0;
}
