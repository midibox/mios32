// $Id$
/*
 * Bank access functions
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
#include "seq_file_b.h"

#include "seq_cc.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"


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

// in which subdirectory of the SD card are the MBSEQ files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define SEQ_FILES_PATH ""
//#define SEQ_FILES_PATH "MySongs/"


/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////

// Structure of bank file:
//    file_type[10]
//    seq_file_b_header_t
//    Pattern 0: seq_file_b_pattern_t
//               Track 0: seq_file_b_track_t
//                        Parameter Layers (num_p_layers * p_layer_size)
//                        Trigger Layers (num_t_layers * t_layer_size)
//               Track 1: ...
//    Pattern 1: ...
//
// Size for 64 patterns, 64*4 tracks,
// each consists of 16 parameter and 8 trigger layers (format is flexible enough to change these numbers)
// parameter layer has 64, trigger layer 256 steps (for easier partitioning, see comments below)
// 10 + 24 + 64 * (24 + 4 * (154 + 16*64 + 8*256/8))
// 10 + 24 + 64 * (24 + 4 * (154 + 1024  + 8*256/8))
// 10 + 24 + 64 * (24 + 4 * 1434)
// 10 + 24 + 64 * (24 + 5736)
// 10 + 24 + 368640
// -> 368674 bytes (good that SD cards are so cheap today ;)

// note: allocating more bytes will result into more FAT operations when seeking a pattern position
// e.g. I tried 16 layers/256 steps, and it took ca. 10 mS to find the starting sector
// With these settings, it takes ca. 3 mS
// The overall loading time is ca. 7 mS - pretty good for such a large bulk of data!!!
// If more steps are desired, just reduce the number of parameter layers
// e.g. 256 steps, 4 parameter layers will result into 1024 bytes as well
// this has the advantage, that a single bank can store different parameter layer configurations so long the size is equal

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
  char name[20];      // track name consists of 20 characters, no zero termination, patted with spaces
  u8   num_p_layers;  // number of parameter layers (usually 3, we will provide more later)
  u8   num_t_layers;  // number of trigger layers (usually 3, we will provide more later)
  u16  p_layer_size;  // size per parameter layer, e.g. 256 steps
  u16  t_layer_size;  // size per trigger layer (divided by 8, as each step has it's own bit)
  u8   cc[128];       // contains all CC parameters, prepared for 128
} seq_file_b_track_t; // 154 bytes



// bank informations stored in RAM
typedef struct {
  unsigned valid: 1;  // bank is accessible

  seq_file_b_header_t header;

  FILEINFO file;      // file informations
} seq_file_b_info_t;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_file_b_info_t seq_file_b_info[SEQ_FILE_B_NUM_BANKS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_Init(u32 mode)
{
  // invalidate all bank infos
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank)
    seq_file_b_info[bank].valid = 0;

  // TODO: move this to a separate function
  // banks should be read when SD Card is plugged into the slot

  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    s32 error;
    error = SEQ_FILE_B_Open(bank);
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] Tried to open bank #%d file, status: %d\n\r", bank+1, error);
#endif
#if 0
    if( error == -2 ) {
      error = SEQ_FILE_B_Create(bank);
#if DEBUG_VERBOSE_LEVEL >= 1
      printf("[SEQ_FILE_B] Tried to create bank #%d file, status: %d\n\r", bank+1, error);
#endif
    }
#endif
  }

  return 0; // no error
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
s32 SEQ_FILE_B_Create(u8 bank)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  seq_file_b_info_t *info = &seq_file_b_info[bank];
  info->valid = 0; // set to invalid so long we are not sure if file can be accessed

  FILEINFO fi;
  int i;

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_B%d.V4", SEQ_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] Creating new bank file '%s'\n\r", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(&fi, filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] Failed to create file, status: %d\n\r", status);
#endif
    return status;
  }

  // write seq_file_b_header
  const char file_type[10] = "MBSEQV4_B";
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)file_type, 10);

  // write bank name w/o zero terminator
  char bank_name[21];
  sprintf(bank_name, "Default Bank        ");
  memcpy(info->header.name, bank_name, 20);
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)info->header.name, 20);
#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] writing '%s'...\n\r", bank_name);
#endif

  // number of patterns
  info->header.num_patterns = 64;
  status |= SEQ_FILE_WriteHWord(&fi, info->header.num_patterns);

  // write predefined pattern size
  u8 num_tracks = 4;
  u8 num_p_layers = 16;
  u16 num_t_layers = 8;
  u16 p_layer_size = 64; // 256 steps if 4 layers
  u16 t_layer_size = 256/8;

  info->header.pattern_size = sizeof(seq_file_b_pattern_t) + 
    num_tracks * (sizeof(seq_file_b_track_t) + num_p_layers*p_layer_size + num_t_layers*t_layer_size);
  status |= SEQ_FILE_WriteHWord(&fi, info->header.pattern_size);

  // close file
  status |= SEQ_FILE_WriteClose(&fi);

  if( status >= 0 ) {
    // bank valid - start to append patterns
    info->valid = 1;

    // appending patterns
    u16 pattern;
    for(pattern=0; pattern<info->header.num_patterns; ++pattern) {
      u8 group = bank % SEQ_CORE_NUM_GROUPS; // note: bank selects source group
      status |= SEQ_FILE_B_PatternWrite(bank, pattern, group);
    }

    // bank invalid again - we have to use SEQ_FILE_B_Open() after a create to init the fileinfo array
    info->valid = 0;
  }

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] Bank file created with status %d\n\r", status);
#endif

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// open a bank file
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_Open(u8 bank)
{
  if( bank >= SEQ_FILE_B_NUM_BANKS )
    return SEQ_FILE_B_ERR_INVALID_BANK;

  seq_file_b_info_t *info = &seq_file_b_info[bank];

  info->valid = 0; // will be set to valid if bank header has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_B%d.V4", SEQ_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] Open bank file '%s'\n\r", filepath);
#endif

  s32 status;
  if( (status=SEQ_FILE_ReadOpen((PFILEINFO)&info->file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] failed to open file, status: %d\n\r", status);
#endif
    return status;
  }

  // read and check header
  // in order to avoid endianess issues, we have to read the sector bytewise!
  char file_type[10];
  if( (status=SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)file_type, 10)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] failed to read header, status: %d\n\r", status);
#endif
    return status;
  }

  if( strncmp(file_type, "MBSEQV4_B", 10) != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    file_type[9] = 0; // ensure that string is terminated
    printf("[SEQ_FILE_B] wrong header type: %s\n\r", file_type);
#endif
    return SEQ_FILE_B_ERR_FORMAT;
  }

  status |= SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)info->header.name, 20);
  status |= SEQ_FILE_ReadHWord((PFILEINFO)&info->file, (u16 *)&info->header.num_patterns);
  status |= SEQ_FILE_ReadHWord((PFILEINFO)&info->file, (u16 *)&info->header.pattern_size);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] file access error while reading header, status: %d\n\r", status);
#endif
    return SEQ_FILE_B_ERR_READ;
  }

  // bank is valid! :)
  info->valid = 1;

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] bank is valid! Number of Patterns: %d, Pattern Size: %d\n\r", info->header.num_patterns, info->header.pattern_size);
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads a pattern from bank into given group
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_PatternRead(u8 bank, u8 pattern, u8 target_group)
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

  // change to file position
  s32 status;
  u32 offset = 10 + sizeof(seq_file_b_header_t) + pattern * info->header.pattern_size;
  if( (status=SEQ_FILE_Seek((PFILEINFO)&info->file, offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n\r", status);
#endif
    return SEQ_FILE_B_ERR_READ;
  }

  status |= SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)seq_pattern_name[target_group], 20);
  seq_pattern_name[target_group][20] = 0;

  u8 num_tracks;
  status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &num_tracks);

  u8 mixer_map;
  status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &mixer_map);

  u8 sysex_setup;
  status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &sysex_setup);

  u8 reserved;
  status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &reserved);

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] read pattern B%d:P%d '%s', %d tracks\n\r", bank+1, pattern, seq_pattern_name[target_group], num_tracks);
#endif

  // reduce number of tracks if required
  if( num_tracks > SEQ_CORE_NUM_TRACKS_PER_GROUP )
    num_tracks = SEQ_CORE_NUM_TRACKS_PER_GROUP;

  u8 track;
  u8 target_track = target_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
  for(track=0; track<num_tracks; ++track, ++target_track) {
    status |= SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)seq_core_trk[target_track].name, 20);
    seq_core_trk[target_track].name[20] = 0;

    u8 given_num_p_layers;
    status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &given_num_p_layers);
    u8 num_p_layers = (given_num_p_layers > SEQ_PAR_NUM_LAYERS) ? SEQ_PAR_NUM_LAYERS : given_num_p_layers;

    u8 given_num_t_layers;
    status |= SEQ_FILE_ReadByte((PFILEINFO)&info->file, &given_num_t_layers);
    u8 num_t_layers = (given_num_t_layers > SEQ_TRG_NUM_LAYERS) ? SEQ_TRG_NUM_LAYERS : given_num_t_layers;

    u16 given_p_layer_size;
    status |= SEQ_FILE_ReadHWord((PFILEINFO)&info->file, &given_p_layer_size);
    u8 p_layer_size = (given_p_layer_size > SEQ_CORE_NUM_STEPS) ? SEQ_CORE_NUM_STEPS : given_p_layer_size;

    u16 given_t_layer_size;
    status |= SEQ_FILE_ReadHWord((PFILEINFO)&info->file, &given_t_layer_size);
    u8 t_layer_size = (given_t_layer_size > (SEQ_CORE_NUM_STEPS/8)) ? (SEQ_CORE_NUM_STEPS/8) : given_t_layer_size;

    u8 cc_buffer[128];
    status |= SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, cc_buffer, 128);
    
    // before changing CCs: we should stop here on error if read failed
    if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      printf("[SEQ_FILE_B] read track #%d (-> %d) failed due to file access error, status: %d\n\r", track+1, target_track+1, status);
#endif
      break;
    }

#if DEBUG_VERBOSE_LEVEL >= 2
    printf("[SEQ_FILE_B] read track #%d (-> %d) '%s', P:%d,T:%d layers (P:%d,T:%d) P:%d,T:%d steps (P:%d,T:%d)\n\r", 
	   track+1, target_track+1,
	   seq_core_trk[target_track].name,
	   num_p_layers, num_t_layers,
	   given_num_p_layers, given_num_t_layers,
	   p_layer_size, 8*t_layer_size,
	   given_p_layer_size, 8*given_t_layer_size);
#endif

    // reading CCs
    u8 cc;
    for(cc=0; cc<128; ++cc)
      SEQ_CC_Set(target_track, cc, cc_buffer[cc]);

    // reading Parameter layers
    if( num_p_layers && p_layer_size ) {
      u8 layer;
      for(layer=0; layer<given_num_p_layers; ++layer) {
	if( layer < num_p_layers ) {
	  SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)&par_layer_value[target_track][layer], p_layer_size);
	  
	  if( p_layer_size < SEQ_CORE_NUM_STEPS ) {
	    // fill remaining bytes with 0
	    memset((u8 *)&par_layer_value[target_track][layer][p_layer_size], 0, SEQ_CORE_NUM_STEPS-p_layer_size);
	  } else if( given_p_layer_size > p_layer_size ) {
	    // read remaining bytes into dummy buffer
	    int i; // read remaining bytes into dummy buffer
	    u8 dummy;
	    for(i=p_layer_size; i<given_p_layer_size; ++i)
	      SEQ_FILE_ReadByte((PFILEINFO)&info->file, &dummy);
	  }
	} else {
	  int i; // read remaining bytes into dummy buffer
	  u8 dummy;
	  for(i=0; i<given_p_layer_size; ++i)
	    SEQ_FILE_ReadByte((PFILEINFO)&info->file, &dummy);
	}
      }
    }
    
    // reading Trigger layers
    if( num_t_layers && t_layer_size ) {
      u8 layer;
      for(layer=0; layer<given_num_t_layers; ++layer) {
	if( layer < num_t_layers ) {
	  SEQ_FILE_ReadBuffer((PFILEINFO)&info->file, (u8 *)&trg_layer_value[target_track][layer], t_layer_size);
	  
	  if( t_layer_size < (SEQ_CORE_NUM_STEPS/8) ) {
	    // fill remaining bytes with 0
	    memset((u8 *)&trg_layer_value[target_track][layer][t_layer_size], 0, (SEQ_CORE_NUM_STEPS/8)-t_layer_size);
	  } else if( given_t_layer_size > t_layer_size ) {
	    // read remaining bytes into dummy buffer
	    int i; // read remaining bytes into dummy buffer
	    u8 dummy;
	    for(i=t_layer_size; i<given_t_layer_size; ++i)
	      SEQ_FILE_ReadByte((PFILEINFO)&info->file, &dummy);
	  }
	} else {
	  int i; // read remaining bytes into dummy buffer
	  u8 dummy;
	  for(i=0; i<given_t_layer_size; ++i)
	    SEQ_FILE_ReadByte((PFILEINFO)&info->file, &dummy);
	}
      }
    }
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] error while reading file, status: %d\n\r", status);
#endif
    return SEQ_FILE_B_ERR_READ;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// writes a pattern of a given group into bank
// returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_B_PatternWrite(u8 bank, u8 pattern, u8 source_group)
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
  u8 num_p_layers = SEQ_PAR_NUM_LAYERS;
  u16 num_t_layers = SEQ_TRG_NUM_LAYERS;
  u16 p_layer_size = SEQ_CORE_NUM_STEPS;
  u16 t_layer_size = SEQ_CORE_NUM_STEPS/8;

  // ok, we should at least check, if the resulting size is within the given range
  u16 expected_pattern_size = sizeof(seq_file_b_pattern_t) + 
    num_tracks * (sizeof(seq_file_b_track_t) + num_p_layers*p_layer_size + num_t_layers*t_layer_size);
  if( expected_pattern_size > info->header.pattern_size ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] Resulting pattern is too large for slot in bank (is: %d, max: %d)\n\r", 
	   expected_pattern_size, info->header.pattern_size);
    return SEQ_FILE_B_ERR_P_TOO_LARGE;
#endif
  }

  FILEINFO fi;

  char filepath[MAX_PATH];
  sprintf(filepath, "%sMBSEQ_B%d.V4", SEQ_FILES_PATH, bank+1);

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] Open bank file '%s' for writing\n\r", filepath);
#endif

  s32 status = 0;
  if( (status=SEQ_FILE_WriteOpen(&fi, filepath, 0)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] Failed to open file, status: %d\n\r", status);
#endif
    SEQ_FILE_WriteClose(&fi); // important to free memory given by malloc
    return status;
  }

  // change to file position
  u32 offset = 10 + sizeof(seq_file_b_header_t) + pattern * info->header.pattern_size;
  if( (status=SEQ_FILE_Seek(&fi, offset)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    printf("[SEQ_FILE_B] failed to change pattern offset in file, status: %d\n\r", status);
#endif
    SEQ_FILE_WriteClose(&fi); // important to free memory given by malloc
    return status;
  }

  // write pattern name w/o zero terminator
  status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)seq_pattern_name[source_group], 20);

#if DEBUG_VERBOSE_LEVEL >= 2
  printf("[SEQ_FILE_B] writing pattern '%s'...\n\r", seq_pattern_name[source_group]);
#endif

  // write number of tracks
  status |= SEQ_FILE_WriteByte(&fi, num_tracks);

  // write link to mixer map
  status |= SEQ_FILE_WriteByte(&fi, 0x00); // off

  // write link to SysEx setup
  status |= SEQ_FILE_WriteByte(&fi, 0x00); // off

  // reserved
  status |= SEQ_FILE_WriteByte(&fi, 0x00);

  // writing tracks
  u8 track;
  u8 source_track = source_group * SEQ_CORE_NUM_TRACKS_PER_GROUP;
  for(track=0; track<num_tracks; ++track, ++source_track) {
    // write track name w/o zero terminator
    status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)seq_core_trk[source_track].name, 20);

    // write number of parameter layers
    status |= SEQ_FILE_WriteByte(&fi, num_p_layers);

    // write number of trigger layers
    status |= SEQ_FILE_WriteByte(&fi, num_t_layers);

    // write size of a single parameter layer
    status |= SEQ_FILE_WriteHWord(&fi, p_layer_size);

    // write size of a single trigger layer
    status |= SEQ_FILE_WriteHWord(&fi, t_layer_size);

    // write 128 CCs
    u8 cc;
    for(cc=0; cc<128; ++cc)
      status |= SEQ_FILE_WriteByte(&fi, SEQ_CC_Get(source_track, cc));

    // write parameter layers
    status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)&par_layer_value[source_track], num_p_layers*p_layer_size);

    // write trigger layers
    status |= SEQ_FILE_WriteBuffer(&fi, (u8 *)&trg_layer_value[source_track], num_t_layers*t_layer_size);
  }

  // fill remaining bytes with zero if required
  if( expected_pattern_size < info->header.pattern_size ) {
    int i;
    for(i=expected_pattern_size; i<info->header.pattern_size; ++i)
      status |= SEQ_FILE_WriteByte(&fi, 0x00);
  }

  // close file
  status |= SEQ_FILE_WriteClose(&fi);

#if DEBUG_VERBOSE_LEVEL >= 1
  printf("[SEQ_FILE_B] Pattern written with status %d\n\r", status);
#endif

  return (status < 0) ? SEQ_FILE_B_ERR_WRITE : 0;
}
