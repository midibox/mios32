// $Id$
/*
 * Song access functions
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
#include "seq_file_s.h"

#include "seq_song.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of song file:
//    file_type[10]
//    seq_file_s_header_t
//    Song 0: seq_file_s_song_header_t
//            seq_song_step_t seq_song_steps[SEQ_SONG_NUM_STEPS]
//    Song 1: ...
//
// Size for 64 songs
// each consists of 128 steps
// 10 + 24 + 64 * (20 + 128 * 8)
// 10 + 24 + 64 * (1044)
// 10 + 24 + 66816
// -> 66850

// not defined as structure: 
// file_type[10] will contain "MBSEQV4_S" + 0 (zero-terminated string)
typedef struct {
  char name[20];      // song bank name consists of 20 characters, no zero termination, patted with spaces
  u16  num_songs;     // number of songs per bank (usually 64)
  u16  song_size;     // reserved size for each song
} seq_file_s_header_t;  // 24 bytes

typedef struct {
  char name[20];      // song name consists of 20 characters, no zero termination, patted with spaces
} seq_file_s_song_header_t; // 20 bytes



// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  seq_file_s_header_t header;

  seq_file_t file;      // file informations
} seq_file_s_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_s_info_t seq_file_s_info;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_Init(u32 mode)
{
  // invalidate bank info
  SEQ_FILE_S_UnloadAllBanks();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_LoadAllBanks(void)
{
  // load bank
  s32 error;
  error = SEQ_FILE_S_Open();
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Tried to open bank file, status: %d\n", error);
#endif
#if 0
  if( error == -2 ) {
    error = SEQ_FILE_S_Create();
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] Tried to create bank file, status: %d\n", error);
#endif
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_UnloadAllBanks(void)
{
  seq_file_s_info.valid = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of songs in bank
// Returns 0 if bank not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_NumSongs(void)
{
  return seq_file_s_info.header.num_songs;
}


/////////////////////////////////////////////////////////////////////////////
// create a complete bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_Create(void)
{
  seq_file_s_info_t *info = &seq_file_s_info;
  info->valid = 0; // set to invalid as long as we are not sure if file can be accessed

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_S.V4", SEQ_FILE_SESSION_PATH, seq_file_session_name);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Creating new bank file '%s'\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] Failed to create file, status: %d\n", status);
#endif
    return status;
  }

  // write seq_file_s_header
  const char file_type[10] = "MBSEQV4_S";
  status |= SEQ_FILE_WriteBuffer((u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= SEQ_FILE_WriteBuffer((u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] writing '%s'...\n", bank_name);
#endif

  // number of songs
  info->header.num_songs = SEQ_SONG_NUM;
  status |= SEQ_FILE_WriteHWord(info->header.num_songs);

  // write predefined song size
  info->header.song_size = sizeof(seq_file_s_song_header_t) + sizeof(seq_song_step_t) * SEQ_SONG_NUM_STEPS;
  status |= SEQ_FILE_WriteHWord(info->header.song_size);

  // not required anymore with FatFs (was required with DOSFS)
#if 0
  // create empty song slots
  u32 song;
  for(song=0; song<info->header.num_songs; ++song) {
    u32 pos;
    for(pos=0; pos<info->header.song_size; ++pos)
      status |= SEQ_FILE_WriteByte(0x00);
  }
#endif

  // close file
  status |= SEQ_FILE_WriteClose();

  if( status >= 0 )
    // bank valid - caller should fill the song slots with useful data now
    info->valid = 1;


#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Bank file created with status %d\n", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_Open(void)
{
  seq_file_s_info_t *info = &seq_file_s_info;

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_S.V4", SEQ_FILE_SESSION_PATH, seq_file_session_name);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Open bank file '%s'\n", filepath);
#endif

  s32 status;
  if( (status=SEQ_FILE_ReadOpen((seq_file_t*)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=SEQ_FILE_ReadBuffer((u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] failed to read header, status: %d\n", status);
#endif

    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);

    return status;
  }

  if( strncmp(file_type, "MBSEQV4_S", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    DEBUG_MSG("[SEQ_FILE_S] wrong header type: %s\n", file_type);
#endif

    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);

    return SEQ_FILE_S_ERR_FORMAT;
  }

  status |= SEQ_FILE_ReadBuffer((u8 *)info->header.name, 20);
  status |= SEQ_FILE_ReadHWord((u16 *)&info->header.num_songs);
  status |= SEQ_FILE_ReadHWord((u16 *)&info->header.song_size);

  // close file (so that it can be re-opened)
  SEQ_FILE_ReadClose((seq_file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] file access error while reading header, status: %d\n", status);
#endif
    return SEQ_FILE_S_ERR_READ;
  }

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] bank is valid! Number of Songs: %d, Song Size: %d\n", info->header.num_songs, info->header.song_size);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a song from bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_SongRead(u8 song)
{
  seq_file_s_info_t *info = &seq_file_s_info;

  if( !info->valid )
    return SEQ_FILE_S_ERR_NO_FILE;

  if( song >= info->header.num_songs )
    return SEQ_FILE_S_ERR_INVALID_SONG;

  // re-open file
  if( SEQ_FILE_ReadReOpen((seq_file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(seq_file_s_header_t) + song * info->header.song_size;
  if( (status=SEQ_FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] failed to change song offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    SEQ_FILE_ReadClose((seq_file_t*)&info->file);
    return SEQ_FILE_S_ERR_READ;
  }

  status |= SEQ_FILE_ReadBuffer((u8 *)seq_song_name, 20);
  seq_song_name[20] = 0;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] read song S%d '%s' (%d bytes)\n", song+1, seq_song_name, info->header.song_size);
#endif

  // reduce size if required
  u16 song_size = info->header.song_size;
  u16 song_size_max = (sizeof(seq_file_s_song_header_t) + sizeof(seq_song_step_t) * SEQ_SONG_NUM_STEPS);
  if( song_size > song_size_max )
    song_size = song_size_max;

  // reading song steps
  u32 entry;
  seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[0];
  u32 num_entries = (song_size - sizeof(seq_file_s_song_header_t)) / sizeof(seq_song_step_t);
  for(entry=0; entry<num_entries; ++entry, ++s) {
    status |= SEQ_FILE_ReadWord((u32 *)&s->ALL_L); // ensure proper endianess - therefore two word reads
    status |= SEQ_FILE_ReadWord((u32 *)&s->ALL_H); // via functions which are aligning the bytes correctly
  }

  // close file (so that it can be re-opened)
  SEQ_FILE_ReadClose((seq_file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] error while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_S_ERR_READ;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a song into bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_S_SongWrite(u8 song, u8 rename_if_empty_name)
{
  seq_file_s_info_t *info = &seq_file_s_info;

  if( !info->valid )
    return SEQ_FILE_S_ERR_FORMAT;

  if( song >= info->header.num_songs )
    return SEQ_FILE_S_ERR_INVALID_SONG;


  // TODO: before writing into song slot, we should check if it already exists, and then
  // compare parameters with given constraints available in following defines/variables:
  u16 song_size = sizeof(seq_file_s_song_header_t) + sizeof(seq_song_step_t) * SEQ_SONG_NUM_STEPS;

  // ok, we should at least check, if the resulting size is within the given range
  if( song_size > info->header.song_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] Resulting song is too large for slot in bank (is: %d, max: %d)\n", 
	   song_size, info->header.song_size);
    return SEQ_FILE_S_ERR_S_TOO_LARGE;
#endif
  }

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_S.V4", SEQ_FILE_SESSION_PATH, seq_file_session_name);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Open bank file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] Failed to open file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(seq_file_s_header_t) + song * info->header.song_size;
  if( (status=SEQ_FILE_WriteSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_S] failed to change song offset in file, status: %d\n", status);
#endif
    SEQ_FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // rename song if name is empty
  if( rename_if_empty_name ) {
    int i;
    u8 found_char = 0;
    for(i=0; i<20; ++i)
      if( seq_song_name[i] != ' ' ) {
	found_char = 1;
	break;
      }

    if( !found_char )
      memcpy(seq_song_name, "Unnamed             ", 20);
  }

  // write song name w/o zero terminator
  status |= SEQ_FILE_WriteBuffer((u8 *)seq_song_name, 20);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_S] writing song #%d '%s'...\n", song+1, seq_song_name);
#endif

  // writing song steps
  u32 entry;
  seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[0];
  u32 num_entries = (song_size - sizeof(seq_file_s_song_header_t)) / sizeof(seq_song_step_t);
  for(entry=0; entry<num_entries; ++entry, ++s) {
    status |= SEQ_FILE_WriteWord(s->ALL_L); // ensure proper endianess - therefore two word writes
    status |= SEQ_FILE_WriteWord(s->ALL_H); // via functions which are aligning the bytes correctly
  }

  // fill remaining bytes with zero if required
  if( song_size < info->header.song_size ) {
    int i;
    for(i=song_size; i<info->header.song_size; ++i)
      status |= SEQ_FILE_WriteByte(0x00);
  }

  // close file
  status |= SEQ_FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_S] Song written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_S_ERR_WRITE : 0;
}
