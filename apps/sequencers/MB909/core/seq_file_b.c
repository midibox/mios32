// $Id: seq_file_b.c 1460 2012-04-05 13:04:37Z tk $
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
#include "seq_file.h"
#include "seq_file_b.h"

#include "seq_core.h"
#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of bank file:
//    file_type[10]
//    seq_file_b_header_t
//    Pattern 0: seq_file_b_pattern_t
//               Track 0: seq_file_b_track_t
//                        Parameter Layers (num_p_instruments * num_p_layers * p_layer_size)
//                        Trigger Layers (num_t_instruments * num_t_layers * t_layer_size)
//               Track 1: ...
//    Pattern 1: ...
//
// Size for 64 patterns, 64*4 tracks,
// each consists of 1 instrument, 16 parameter and 8 trigger layers (format is flexible enough to change these numbers)
// parameter layer has 64, trigger layer 256 steps (for easier partitioning, see comments below)
// 10 + 24 + 64 * (24 + 4 * (216 + 1*16*64 + 1*8*256/8))
// 10 + 24 + 64 * (24 + 4 * (216 + 1024  + 256))
// 10 + 24 + 64 * (24 + 4 * 1496)
// 10 + 24 + 64 * (24 + 5984)
// 10 + 24 + 384512
// -> 384546 bytes (good that SD cards are so cheap today ;)

// note: allocating more bytes will result into more FAT operations when seeking a pattern position
// e.g. I tried 16 layers/256 steps, and it took ca. 10 mS to find the starting sector
// With these settings, it takes ca. 3 mS
// The overall loading time is ca. 7 mS - pretty good for such a large bulk of data!!!
// If more steps are desired, just reduce the number of parameter layers
// e.g. 256 steps, 4 parameter layers will result into 1024 bytes as well
// this has the advantage, that a single bank can store different parameter layer configurations as long as the size is equal

// not defined as structure: 
// file_type[10] will contain "MBSEQV4_B" + 0 (zero-terminated string)
typedef struct {
  char name[20];      // bank name consists of 20 characters, no zero termination, patted with spaces
  u16  num_patterns;  // number of patterns per bank (usually 64)
  u16  pattern_size;  // reserved size for each pattern
} seq_file_b_header_t;  // 24 bytes

typedef struct {
  char name[20];      // pattern name consists of 20 characters, no zero termination, patted with spaces
  u8   num_tracks;    // number of tracks in pattern (usually 4)
  u8   mixer_map;     // link to optional mixer map (0=off)
  u8   sysex_setup;   // link to optional sysex setup (0=off)
  u8   reserved1;     // reserved for future extensions
} seq_file_b_pattern_t;  // 24 bytes

typedef struct {
  char name[80];      // track name consists of 80 characters, no zero termination, patted with spaces
  u8   num_p_instruments; // number of instruments (usually 1, or 16/8 for drum tracks)
  u8   num_t_instruments; // number of instruments (usually 1, or 16/8 for drum tracks)
  u8   num_p_layers;  // number of parameter layers (usually 4, 8 or 16)
  u8   num_t_layers;  // number of trigger layers (usually 8, or 1/2 for drum tracks)
  u16  p_layer_size;  // size per parameter layer, e.g. 256 steps
  u16  t_layer_size;  // size per trigger layer (divided by 8, as each step has it's own bit)
  u8   cc[128];       // contains all CC parameters, prepared for 128
} seq_file_b_track_t; // 156 bytes



// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  seq_file_b_header_t header;

  file_t file;      // file informations
} seq_file_b_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_b_info_t seq_file_b_info[SEQ_FILE_B_NUM_BANKS];

static u8 cached_pattern_name[21];
static u8 cached_bank;
static u8 cached_pattern;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_Init(u32 mode)
{
  // invalidate all bank infos
  SEQ_FILE_B_UnloadAllBanks();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been connected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_LoadAllBanks(char *session)
{
  s32 status = 0;

  // load all banks
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    s32 error = SEQ_FILE_B_Open(session, bank);
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] Tried to open bank #%d file, status: %d\n", bank+1, error);
#endif
#if 0
    if( error == -2 ) {
      error = SEQ_FILE_B_Create(session, bank);
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_B] Tried to create bank #%d file, status: %d\n", bank+1, error);
#endif
    }
#endif
    status |= error;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Unloads all banks
// Called from SEQ_FILE_CheckSDCard() when the SD card has been disconnected
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_UnloadAllBanks(void)
{
  // invalidate all bank infos
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank)
    seq_file_b_info[bank].valid = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Saves all banks
// returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_SaveAllBanks(char *session)
{
  s32 status = 0;

  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    s32 error = SEQ_FILE_B_PatternWrite(seq_file_session_name, seq_pattern[bank].bank, seq_pattern[bank].pattern, bank, 1);

#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] Tried to store bank #%d file, status: %d\n", bank+1, error);
#endif

    status |= error;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of patterns in bank
// Returns 0 if bank not valid
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_NumPatterns(u8 bank)
{
  if( (bank < SEQ_FILE_B_NUM_BANKS) && seq_file_b_info[bank].valid )
    return seq_file_b_info[bank].header.num_patterns;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// create a complete bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_Create(char *session, u8 bank)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  seq_file_b_info_t *info = &seq_file_b_info[bank];
  info->valid = 0; // set to invalid as long as we are not sure if file can be accessed

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_B%d.V4", SEQ_FILE_SESSION_PATH, session, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] Creating new bank file '%s'\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] Failed to create file, status: %d\n", status);
#endif
    return status;
  }

  // write seq_file_b_header
  const char file_type[10] = "MBSEQV4_B";
  status |= FILE_WriteBuffer((u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= FILE_WriteBuffer((u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] writing '%s'...\n", bank_name);
#endif

  // number of patterns
  info->header.num_patterns = 64;
  status |= FILE_WriteHWord(info->header.num_patterns);

  // write predefined pattern size
  u8 num_tracks = 4;
  u8 num_p_instruments = 1;
  u8 num_t_instruments = 1;
  u8 num_p_layers = 16;
  u16 num_t_layers = 8;
  u16 p_layer_size = 64; // 256 steps if 4 layers
  u16 t_layer_size = 256/8;

  info->header.pattern_size = sizeof(seq_file_b_pattern_t) + 
    num_tracks * (sizeof(seq_file_b_track_t) + num_p_instruments*num_p_layers*p_layer_size + num_t_instruments*num_t_layers*t_layer_size);
  status |= FILE_WriteHWord(info->header.pattern_size);

  // not required anymore with FatFs (was required with DOSFS)
#if 0
  // create empty pattern slots
  u32 pattern;
  for(pattern=0; pattern<info->header.num_patterns; ++pattern) {
    u32 pos;
    for(pos=0; pos<info->header.pattern_size; ++pos)
      status |= FILE_WriteByte(0x00);
  }
#endif

  // close file
  status |= FILE_WriteClose();

  if( status >= 0 )
    // bank valid - caller should fill the pattern slots with useful data now
    info->valid = 1;


#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] Bank file created with status %d\n", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_Open(char *session, u8 bank)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  seq_file_b_info_t *info = &seq_file_b_info[bank];

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_B%d.V4", SEQ_FILE_SESSION_PATH, session, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] Open bank file '%s'\n", filepath);
#endif

  s32 status;
  if( (status=FILE_ReadOpen((file_t*)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=FILE_ReadBuffer((u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] failed to read header, status: %d\n", status);
#endif
    return status;
  }

  if( strncmp(file_type, "MBSEQV4_B", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    DEBUG_MSG("[SEQ_FILE_B] wrong header type: %s\n", file_type);
#endif
    return SEQ_FILE_B_ERR_FORMAT;
  }

  status |= FILE_ReadBuffer((u8 *)info->header.name, 20);
  status |= FILE_ReadHWord((u16 *)&info->header.num_patterns);
  status |= FILE_ReadHWord((u16 *)&info->header.pattern_size);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] file access error while reading header, status: %d\n", status);
#endif
    return SEQ_FILE_B_ERR_READ;
  }

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] bank is valid! Number of Patterns: %d, Pattern Size: %d\n", info->header.num_patterns, info->header.pattern_size);
#endif

  // finally (re-)load cached pattern name - status of this function doesn't matter
  char dummy[21];
  SEQ_FILE_B_PatternPeekName(cached_bank, cached_pattern, 1, dummy); // non-cached!

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a pattern from bank into given group
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_PatternRead(u8 bank, u8 pattern, u8 target_group, u16 remix_map)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  if( target_group >= SEQ_CORE_NUM_GROUPS )
    return SEQ_FILE_B_ERR_INVALID_GROUP;

  seq_file_b_info_t *info = &seq_file_b_info[bank];

  if( !info->valid )
    return SEQ_FILE_B_ERR_NO_FILE;

  if( pattern >= info->header.num_patterns )
    return SEQ_FILE_B_ERR_INVALID_PATTERN;

  // re-open file
  if( FILE_ReadReOpen((file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(seq_file_b_header_t) + pattern * info->header.pattern_size;
  if( (status=FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    FILE_ReadClose((file_t*)&info->file);
    return SEQ_FILE_B_ERR_READ;
  }

  status |= FILE_ReadBuffer((u8 *)seq_pattern_name[target_group], 20);
  seq_pattern_name[target_group][20] = 0;

  u8 num_tracks;
  status |= FILE_ReadByte(&num_tracks);

  u8 mixer_map;
  status |= FILE_ReadByte(&mixer_map);

  u8 sysex_setup;
  status |= FILE_ReadByte(&sysex_setup);

  u8 reserved;
  status |= FILE_ReadByte(&reserved);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] read pattern B%d:P%d '%s', %d tracks\n", bank+1, pattern, seq_pattern_name[target_group], num_tracks);
#endif

  // reduce number of tracks if required
  if( num_tracks > SEQ_CORE_NUM_TRACKS_PER_GROUP )
    num_tracks = SEQ_CORE_NUM_TRACKS_PER_GROUP;

  u8 track_i;
  u8 track = target_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
  for(track_i=0; track_i<num_tracks; ++track_i, ++track) {
		
    // if we got the track bit setup inside our remix_map, them do not change him, let it be mixed down
    if ( ((1 << track) | remix_map) == remix_map ) {
      // Mixed down! no need to change the track pattern
      // but we need to state our file pointer... jump to the next track data

DEBUG_MSG("Skipping Track %d\n", track);
      u8 dummy_name[80];
      status |= FILE_ReadBuffer(dummy_name, 80); // dummy! don't take over track name

      u8 num_p_instruments;
      status |= FILE_ReadByte(&num_p_instruments);

      u8 num_t_instruments;
      status |= FILE_ReadByte(&num_t_instruments);

      u8 num_p_layers;
      status |= FILE_ReadByte(&num_p_layers);

      u8 num_t_layers;
      status |= FILE_ReadByte(&num_t_layers);

      u16 p_layer_size;
      status |= FILE_ReadHWord(&p_layer_size);

      u16 t_layer_size;
      status |= FILE_ReadHWord(&t_layer_size);

      // skip CC and Par/Trg layer
      u32 par_size = num_p_instruments * num_p_layers * p_layer_size;
      u32 trg_size = num_t_instruments * num_t_layers * t_layer_size;
      u32 new_pos = FILE_ReadGetCurrentPosition() + 128 + par_size + trg_size;
 DEBUG_MSG("Pos change: %d -> %d\n", FILE_ReadGetCurrentPosition(), new_pos);
      if( (status=FILE_ReadSeek(new_pos)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n", status);
#endif
	// close file (so that it can be re-opened)
	FILE_ReadClose((file_t*)&info->file);
	return SEQ_FILE_B_ERR_READ;
      }
 DEBUG_MSG("New pos: %d\n", FILE_ReadGetCurrentPosition());

    } else {
			
      status |= FILE_ReadBuffer((u8 *)seq_core_trk[track].name, 80);
      seq_core_trk[track].name[80] = 0;

      u8 num_p_instruments;
      status |= FILE_ReadByte(&num_p_instruments);

      u8 num_t_instruments;
      status |= FILE_ReadByte(&num_t_instruments);

      u8 num_p_layers;
      status |= FILE_ReadByte(&num_p_layers);

      u8 num_t_layers;
      status |= FILE_ReadByte(&num_t_layers);

      u16 p_layer_size;
      status |= FILE_ReadHWord(&p_layer_size);

      u16 t_layer_size;
      status |= FILE_ReadHWord(&t_layer_size);

      u8 cc_buffer[128];
      status |= FILE_ReadBuffer(cc_buffer, 128);
    
      // before changing CCs: we should stop here on error if read failed
      if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[SEQ_FILE_B] read track #%d (-> %d) failed due to file access error, status: %d\n", track+1, track+1, status);
#endif
	break;
      }

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SEQ_FILE_B] read track #%d (-> %d) '%s'\n", track+1, track+1, seq_core_trk[track].name);
      DEBUG_MSG("[SEQ_FILE_B] P:%d,T:%d instruments P:%d,T:%d layers P:%d,T:%d steps\n", 
		num_p_instruments, num_t_instruments,
		num_p_layers, num_t_layers,
		p_layer_size, 8*t_layer_size);
#endif

      // reading CCs
      u8 cc;
      for(cc=0; cc<128; ++cc)
	SEQ_CC_Set(track, cc, cc_buffer[cc]);

      // partitionate parameter layer and clear all steps
      SEQ_PAR_TrackInit(track, p_layer_size, num_p_layers, num_p_instruments);

      // reading Parameter layers
      u32 par_size = num_p_instruments * num_p_layers * p_layer_size;
      u32 par_size_taken = (par_size > SEQ_PAR_MAX_BYTES) ? SEQ_PAR_MAX_BYTES : par_size;
      if( par_size_taken )
	FILE_ReadBuffer((u8 *)&seq_par_layer_value[track], par_size_taken);

      // read remaining bytes into dummy buffer
      while( par_size > par_size_taken ) {
	u8 dummy;
	FILE_ReadByte(&dummy);
	++par_size_taken;
      }

      // partitionate trigger layer and clear all steps
      SEQ_TRG_TrackInit(track, t_layer_size*8, num_t_layers, num_t_instruments);

      // reading Trigger layers
      u32 trg_size = num_t_instruments * num_t_layers * t_layer_size;
      u32 trg_size_taken = (trg_size > SEQ_TRG_MAX_BYTES) ? SEQ_TRG_MAX_BYTES : trg_size;
      if( trg_size_taken )
	FILE_ReadBuffer((u8 *)&seq_trg_layer_value[track], trg_size_taken);

      // read remaining bytes into dummy buffer
      while( trg_size > trg_size_taken ) {
	u8 dummy;
	FILE_ReadByte(&dummy);
	++trg_size_taken;
      }

      // finally update CC links again, because some of them depend on SEQ_PAR_NumLayersGet()!!!
      SEQ_CC_LinkUpdate(track);

    }
  }

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] error while reading file, status: %d\n", status);
#endif
    return SEQ_FILE_B_ERR_READ;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a pattern of a given group into bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_PatternWrite(char *session, u8 bank, u8 pattern, u8 source_group, u8 rename_if_empty_name)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  if( source_group >= SEQ_CORE_NUM_GROUPS )
    return SEQ_FILE_B_ERR_INVALID_GROUP;

  seq_file_b_info_t *info = &seq_file_b_info[bank];

  if( !info->valid )
    return SEQ_FILE_B_ERR_FORMAT;

  if( pattern >= info->header.num_patterns )
    return SEQ_FILE_B_ERR_INVALID_PATTERN;


  // TODO: before writing into pattern slot, we should check if it already exists, and then
  // compare layer parameters with given constraints available in following defines/variables:
  u8 num_tracks = SEQ_CORE_NUM_TRACKS_PER_GROUP;

  // ok, we should at least check, if the resulting size is within the given range
  u16 expected_pattern_size = sizeof(seq_file_b_pattern_t);

  u8 track = source_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
  u8 track_i;
  for(track_i=0; track_i<num_tracks; ++track_i, ++track) {
    u8 num_p_instruments = SEQ_PAR_NumInstrumentsGet(track);
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    u16 p_layer_size = SEQ_PAR_NumStepsGet(track);
    u8 num_t_instruments = SEQ_TRG_NumInstrumentsGet(track);
    u8 num_t_layers = SEQ_TRG_NumLayersGet(track);
    u16 t_layer_size = SEQ_TRG_NumStepsGet(track)/8;

    expected_pattern_size += sizeof(seq_file_b_track_t) + 
      num_p_instruments*num_p_layers*p_layer_size + 
      num_t_instruments*num_t_layers*t_layer_size;
  }

  if( expected_pattern_size > info->header.pattern_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] Resulting pattern is too large for slot in bank (is: %d, max: %d)\n", 
	   expected_pattern_size, info->header.pattern_size);
    return SEQ_FILE_B_ERR_P_TOO_LARGE;
#endif
  }

  char filepath[MAX_PATH];
  sprintf(filepath, "%s/%s/MBSEQ_B%d.V4", SEQ_FILE_SESSION_PATH, session, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] Open bank file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] Failed to open file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(seq_file_b_header_t) + pattern * info->header.pattern_size;
  if( (status=FILE_WriteSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    return status;
  }

  // rename pattern if name is empty
  if( rename_if_empty_name ) {
    int i;
    u8 found_char = 0;
    u8 *label = (u8 *)&seq_pattern_name[source_group][5];
    for(i=0; i<15; ++i)
      if( label[i] != ' ' ) {
	found_char = 1;
	break;
      }

    if( !found_char )
      memcpy(label, "Unnamed        ", 15);
  }

  // write pattern name w/o zero terminator
  status |= FILE_WriteBuffer((u8 *)seq_pattern_name[source_group], 20);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_B] writing pattern '%s'...\n", seq_pattern_name[source_group]);
#endif

  // write number of tracks
  status |= FILE_WriteByte(num_tracks);

  // write link to mixer map
  status |= FILE_WriteByte(0x00); // off

  // write link to SysEx setup
  status |= FILE_WriteByte(0x00); // off

  // reserved
  status |= FILE_WriteByte(0x00);

  // writing tracks
  track = source_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
  for(track_i=0; track_i<num_tracks; ++track_i, ++track) {
    u8 num_p_instruments = SEQ_PAR_NumInstrumentsGet(track);
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    u16 p_layer_size = SEQ_PAR_NumStepsGet(track);
    u8 num_t_instruments = SEQ_TRG_NumInstrumentsGet(track);
    u8 num_t_layers = SEQ_TRG_NumLayersGet(track);
    u16 t_layer_size = SEQ_TRG_NumStepsGet(track)/8;

    // write track name w/o zero terminator
    status |= FILE_WriteBuffer((u8 *)seq_core_trk[track].name, 80);

    // write number of parameter instruments
    status |= FILE_WriteByte(num_p_instruments);

    // write number of trigger instruments
    status |= FILE_WriteByte(num_t_instruments);

    // write number of parameter layers
    status |= FILE_WriteByte(num_p_layers);

    // write number of trigger layers
    status |= FILE_WriteByte(num_t_layers);

    // write size of a single parameter layer
    status |= FILE_WriteHWord(p_layer_size);

    // write size of a single trigger layer
    status |= FILE_WriteHWord(t_layer_size);

    // write 128 CCs
    u8 cc;
    for(cc=0; cc<128; ++cc) {
      s32 cc_value = SEQ_CC_Get(track, cc);
      if( cc_value < 0 ) // set CC value to 0 if it doesn't exist (reserved CCs)
	cc_value = 0;
      status |= FILE_WriteByte(cc_value);
    }

    // write parameter layers
    status |= FILE_WriteBuffer((u8 *)&seq_par_layer_value[track], num_p_instruments*num_p_layers*p_layer_size);

    // write trigger layers
    status |= FILE_WriteBuffer((u8 *)&seq_trg_layer_value[track], num_t_instruments*num_t_layers*t_layer_size);
  }

  // fill remaining bytes with zero if required
  while( expected_pattern_size < info->header.pattern_size ) {
    status |= FILE_WriteByte(0x00);
    ++expected_pattern_size;
  }

  // close file
  status |= FILE_WriteClose();

#if DEBUG_VERBOSE_LEVEL >= 1
  DEBUG_MSG("[SEQ_FILE_B] Pattern written with status %d\n", status);
#endif

  return (status < 0) ? SEQ_FILE_B_ERR_WRITE : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns a pattern name from disk w/o overwriting patterns in RAM
//
// used in SAVE menu to display the pattern name which will be overwritten
// 
// function can be called frequently w/o performance loss, as the name
// of bank/pattern will be cached.
// non_cached=1 forces an update regardless of bank/pattern number
//
// *name will contain 20 characters + 0 terminator regardless of status
//
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_PatternPeekName(u8 bank, u8 pattern, u8 non_cached, char *pattern_name)
{
  if( !non_cached && cached_bank == bank && cached_pattern == pattern ) {
    // name is in cache
    memcpy(pattern_name, cached_pattern_name, 21);
    return 0; // no error
  }

  cached_bank = bank;
  cached_pattern = pattern;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_B] Loading Pattern Name for %d:%d\n", bank, pattern);
#endif

  // initial pattern name
  memcpy(cached_pattern_name, "-----<Disk Error>   ", 21);

  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  seq_file_b_info_t *info = &seq_file_b_info[bank];

  if( !info->valid )
    return SEQ_FILE_B_ERR_NO_FILE;

  if( pattern >= info->header.num_patterns )
    return SEQ_FILE_B_ERR_INVALID_PATTERN;

  // re-open file
  if( FILE_ReadReOpen((file_t*)&info->file) < 0 )
    return -1; // file cannot be re-opened

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(seq_file_b_header_t) + pattern * info->header.pattern_size;
  if( (status=FILE_ReadSeek(offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n", status);
#endif
    // close file (so that it can be re-opened)
    FILE_ReadClose((file_t*)&info->file);
    return SEQ_FILE_B_ERR_READ;
  }

  // read name
  status |= FILE_ReadBuffer((u8 *)cached_pattern_name, 20);
  cached_pattern_name[20] = 0;

  // close file (so that it can be re-opened)
  FILE_ReadClose((file_t*)&info->file);

  // fill category with "-----" if it is empty
  int i;
  u8 found_char = 0;
  for(i=0; i<5; ++i)
    if( cached_pattern_name[i] != ' ' ) {
      found_char = 1;
      break;
    }
  if( !found_char )
    memcpy(&cached_pattern_name[0], "-----", 5);


  // fill label with "<empty>" if it is empty
  found_char = 0;
  for(i=5; i<20; ++i)
    if( cached_pattern_name[i] != ' ' ) {
      found_char = 1;
      break;
    }
  if( !found_char )
    memcpy(&cached_pattern_name[5], "<empty>        ", 15);

  
  // copy into return variable
  memcpy(pattern_name, cached_pattern_name, 21);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_B] Loading Pattern Name for %d:%d successfull\n", bank, pattern);
#endif

  return 0; // no error
}
