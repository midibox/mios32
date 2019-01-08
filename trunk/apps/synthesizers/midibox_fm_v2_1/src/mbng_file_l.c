// $Id: mbng_file_l.c 1758 2013-04-23 20:42:55Z tk $
//! \defgroup MBNG_FILE_L
//! Label File access functions
//! 
//! NOTE: before accessing the SD Card, the upper level function should
//! synchronize with the SD Card semaphore!
//!   MUTEX_SDCARD_TAKE; // to take the semaphore
//!   MUTEX_SDCARD_GIVE; // to release the semaphore
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
//! Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "tasks.h"

#include <string.h>
#include <md5.h>

#include "file.h"
#include "mbng_file.h"
#include "mbng_file_l.h"


/////////////////////////////////////////////////////////////////////////////
//! for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages!
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
//! Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the files located?
// use "/" for root
// use "/<dir>/" for a subdirectory in root
// use "/<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MBNG_FILES_PATH "/"
//#define MBNG_FILES_PATH "/MySongs/"


// format of the .bin file (32bit) - has to be changed whenever the .bin structure changes!
#define BIN_FILE_FORMAT_NUMBER 0

/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
  file_t bin_file;
  u16 num_labels;
  u32 label_pos[MBNG_FILE_L_NUM_LABELS];
  char labels[MBNG_FILE_L_NUM_LABELS][8];
  u8 label_buffer[256];
} mbng_file_l_info_t;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

#ifndef AHB_SECTION
#define AHB_SECTION
#endif
static AHB_SECTION mbng_file_l_info_t mbng_file_l_info;


/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////
char mbng_file_l_patch_name[MBNG_FILE_L_FILENAME_LEN+1];


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_L_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Loads patch file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been connected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Load(char *filename)
{
  s32 error;
  error = MBNG_FILE_L_Read(filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_L] Tried to open label file %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! Unloads label file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been disconnected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Unload(void)
{
  mbng_file_l_info.valid = 0;
  mbng_file_l_info.num_labels = 0;
  mbng_file_l_info.label_buffer[0] = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \returns 1 if current label file valid
//! \returns 0 if current label file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Valid(void)
{
  return mbng_file_l_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
//! read the .BIN file and checks if it is valid
//! md5_checksum_binfile[16] contains the expected MD5 checksum of the .NGL file if valid
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_L_Read - this will blow up the stack usage too much!
s32 readBinFileMD5(char *filepath, u8 md5_checksum[16])
{
  s32 status;

  if( (status=FILE_ReadOpen(&mbng_file_l_info.bin_file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_L] %s doesn't exist\n", filepath);
#endif
    return status;
  }

  u32 format;
  if( (status=FILE_ReadWord(&format)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while reading %s - compiling new one\n", filepath);
#endif
    FILE_ReadClose(&mbng_file_l_info.bin_file);
    return -1;
  }

  if( format != BIN_FILE_FORMAT_NUMBER ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] WARNING: .BIN file format has been changed - compiling new one\n");
#endif
    FILE_ReadClose(&mbng_file_l_info.bin_file);
    return -2;
  }

  // read checksum
  if( (status=FILE_ReadBuffer(md5_checksum, 16)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while reading %s - compiling new one\n", filepath);
#endif
  } else {
    status = 1; // success
  }
  FILE_ReadClose(&mbng_file_l_info.bin_file);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! parse the .BIN file for labels
//! \eturns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_L_Read - this will blow up the stack usage too much!
s32 parseBinFile(char *filepath)
{
  s32 status;
  u32 pos = 4 + 16; // start to read after format and MD5 checksum

  mbng_file_l_info_t *info = &mbng_file_l_info;
  info->valid = 1;

  if( (status=FILE_ReadOpen(&info->bin_file, filepath)) < 0 ||
      (status=FILE_ReadSeek(pos)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] failed to open %s again - compiling new one\n", filepath);
#endif
    return status;
  }

  for( ; info->num_labels<MBNG_FILE_L_NUM_LABELS && info->valid && pos < FILE_ReadGetCurrentSize() ; ) {
    u8 len;
    u8 label_type;
    u8 label_len;
    char label[9];
    {
      int i;
      for(i=0; i<9; ++i)
	label[i] = 0;
    }

    if( (status=FILE_ReadByte(&len)) < 0 ||
	(status=FILE_ReadByte(&label_type)) < 0 ||
	(status=FILE_ReadByte(&label_len)) < 0 ||
	(status=FILE_ReadBuffer((u8 *)&label, label_len)) < 0
	) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while reading .BIN file - compiling new one\n");
#endif
      info->valid = 0;
    } else if( len == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_L] WARNING: unexpected record with zero length found in .BIN file - please report to TK");
#endif
      break; // failsafe
    } else if( label_type >= 10 ) { // not stored in info->label_pos
      // next item
      pos += len;
      FILE_ReadSeek(pos);
    } else {
      info->label_pos[info->num_labels] = pos;
      memcpy(info->labels[info->num_labels], label, 8);
      if( label_len < 8 )
	info->labels[info->num_labels][label_len] = 0;

      ++info->num_labels;

      // next item
      pos += len;
      FILE_ReadSeek(pos);
    }
  }

  FILE_ReadClose(&info->bin_file);

  if( info->valid ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] %d labels found in %s file%s\n", info->num_labels, filepath, (info->num_labels >= MBNG_FILE_L_NUM_LABELS) ? " (maximum reached)" : "");
#endif
  }

  return info->valid ? 0 : -1;
}


/////////////////////////////////////////////////////////////////////////////
//! creates the MD5 checksum over the .NGL file
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
//static // TK: removed static to avoid inlining in MBNG_FILE_L_Read - this will blow up the stack usage too much!
s32 generateNglFileMD5(char *filepath, u8 md5_checksum[16])
{
  s32 status;
  file_t file;

  // open .ngl file
  {
    if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[MBNG_FILE_L] failed to open file, status: %d\n", status);
#endif
      return status;
    }
  }

  {
#define MD5_READ_BLOCKSIZE 64 // must be dividable by 64
    u8 buffer[MD5_READ_BLOCKSIZE];
    struct md5_ctx ctx;
    md5_init_ctx(&ctx);

    s32 len = 0;
    while( 1 ) {
      if( (len=FILE_ReadBufferUnknownLen(buffer, MD5_READ_BLOCKSIZE)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_L] failed to read file, status: %d\n", status);
#endif
	FILE_ReadClose(&file);
	return len; // contains error status
      }

      if( len != MD5_READ_BLOCKSIZE )
	break;

      md5_process_block(buffer, MD5_READ_BLOCKSIZE, &ctx);
    }

    if( len > 0 )
      md5_process_bytes(buffer, len, &ctx);
    
    md5_finish_ctx(&ctx, md5_checksum);
  }

  FILE_ReadClose(&file);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_L] MD5 checksum:\n");
  MIOS32_MIDI_SendDebugHexDump(md5_checksum, 16);
#endif

  return 1; // file valid
}


/////////////////////////////////////////////////////////////////////////////
//! help function which parses a decimal or hex value
//! \returns >= 0 if value is valid
//! \returns -1000000000 if value is invalid
/////////////////////////////////////////////////////////////////////////////
static s32 get_dec(char *word)
{
  if( word == NULL )
    return -1000000000;

  char *next;
  long l = strtol(word, &next, 0);

  if( word == next )
    return -1000000000;

  return l; // value is valid
}

/////////////////////////////////////////////////////////////////////////////
//! help function which parses a quoted string
//! \returns >= 0 if string is valid
//! \returns < 0 if string is invalid
/////////////////////////////////////////////////////////////////////////////
static char *getQuotedString(char **brkt)
{
  char *value_str;

  if( *brkt == NULL )
    return NULL;

  value_str = *brkt;
  {
    int quote_started = 0;

    char *search_str = *brkt;
    for(; *search_str != 0 && (*search_str == ' ' || *search_str == '\t'); ++search_str);

    if( *search_str == '\'' || *search_str == '"' ) {
      quote_started = *search_str == '\'' ? 1 : 2;
      ++search_str;
    }

    value_str = search_str;

    if( quote_started == 1 ) {
      for(; *search_str != 0 && *search_str != '\''; ++search_str);
    } else if( quote_started ) {
      for(; *search_str != 0 && *search_str != '\"'; ++search_str);
    } else {
      for(; *search_str != 0 && *search_str != ' ' && *search_str != '\t'; ++search_str);
    }

    if( *search_str != 0 ) {
      *search_str = 0;
      ++search_str;
    }

    *brkt = search_str;
  }

  // end of command line reached?
  if( (*brkt)[0] == 0 )
    *brkt = NULL;

  return value_str[0] ? value_str : NULL;
}

/////////////////////////////////////////////////////////////////////////////
//! reads the label file content (again)
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
  char filepath[MAX_PATH];
s32 MBNG_FILE_L_Read(char *filename)
{
  s32 status = 0;
  mbng_file_l_info_t *info = &mbng_file_l_info;

  info->valid = 0; // will be set to valid if file content has been read successfully
  info->label_buffer[0] = 0;
  info->num_labels = 0;

  // store current file name in global variable for UI
  memcpy(mbng_file_l_patch_name, filename, MBNG_FILE_L_FILENAME_LEN+1);

  // check if compiled .BIN file exists and if it has right format
  u8 md5_checksum_binfile[16];
  sprintf(filepath, "%s%s.BIN", MBNG_FILES_PATH, filename);
  u8 bin_file_valid = readBinFileMD5(filepath, md5_checksum_binfile) >= 0;

  // determine the MD5 checksum of the .NGL file
  u8 md5_checksum_nglfile[16];
  sprintf(filepath, "%s%s.NGL", MBNG_FILES_PATH, filename);
  if( (status=generateNglFileMD5(filepath, md5_checksum_nglfile)) < 0 ) {
    return status; // error already reported
  }

  // if it doesn't match, then we've to generate a new .bin file
  if( bin_file_valid ) {
    // compare checksum
    int i;
    u8 checksum_matches = 1;
    for(i=0; i<16 && checksum_matches; ++i) 
      if( md5_checksum_nglfile[i] != md5_checksum_binfile[i] )
	checksum_matches = 0;

    if( checksum_matches ) {
      DEBUG_MSG("[MBNG_FILE_L] %s.NGL hasn't been changed; reading labels from existing %s.BIN file.\n", filename, filename);
    } else {
      bin_file_valid = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_L] WARNING: .NGL content doesn't match with .BIN file - compiling new one\n");
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[MBNG_FILE_L] MD5 checksum of .NGL file:\n");
      MIOS32_MIDI_SendDebugHexDump(md5_checksum_nglfile, 16);
      DEBUG_MSG("[MBNG_FILE_L] MD5 checksum of .BIN file:\n");
      MIOS32_MIDI_SendDebugHexDump(md5_checksum_binfile, 16);
#endif
#endif
    }
  }

  // parse .bin file for label positions
  sprintf(filepath, "%s%s.BIN", MBNG_FILES_PATH, filename);
  if( bin_file_valid && parseBinFile(filepath) >= 0 ) {
    return 0; // -> done
  }
  bin_file_valid = 0;
  
  // create new .BIN file
  {
    sprintf(filepath, "%s%s.BIN", MBNG_FILES_PATH, mbng_file_l_patch_name);
    if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_L] ERROR: failed to create a new %s file!\n", filepath);
#endif
      FILE_WriteClose(); // important to free memory given by malloc
      return status;
    } else {
      if( (status=FILE_WriteWord(BIN_FILE_FORMAT_NUMBER)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while writing %s!\n", filepath);
#endif
	return status;
      }

      if( (status=FILE_WriteBuffer(md5_checksum_nglfile, 16)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while writing %s!\n", filepath);
#endif
	return status;
      }
    }
  }

  // open .ngl file again
  file_t file;
  sprintf(filepath, "%s%s.NGL", MBNG_FILES_PATH, mbng_file_l_patch_name);
  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_L] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read label definitions
  u32 line_buffer_size = 1024;
  char *line_buffer = pvPortMalloc(line_buffer_size);
  u32 line_buffer_len = 0;
  if( !line_buffer ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] FATAL: out of heap memory!\n");
#endif
    return -1;
  }

  u32 bin_file_errors = 0;
  char current_cond_label[9];
  current_cond_label[0] = 0;
  int num_labels = 0;
  u32 line = 0;
  do {
    ++line;
    status=FILE_ReadLine((u8 *)(line_buffer+line_buffer_len), line_buffer_size-line_buffer_len);

    if( status > 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 3
      if( line_buffer_len )
	MIOS32_MIDI_SendDebugString("+++");
      MIOS32_MIDI_SendDebugString(line_buffer);
#endif

      // concatenate?
      u32 new_len = strlen(line_buffer);
      // remove spaces
      while( new_len >= 1 && line_buffer[new_len-1] == ' ' ) {
	line_buffer[new_len-1] = 0;
	--new_len;
      }
      if( new_len >= 1 && line_buffer[new_len-1] == '\\' ) {
	line_buffer[new_len-1] = 0;
	line_buffer_len = new_len - 1;
	continue; // read next line
      } else {
	line_buffer_len = 0; // for next round we start at 0 again
      }

      // sscanf consumes too much memory, therefore we parse directly
      char *separators = " \t;";
      char *brkt;
      char *parameter;

      if( (parameter = strtok_r(line_buffer, separators, &brkt)) ) {

	if( *parameter == 0 || *parameter == '#' ) {
	  // ignore comments and empty lines
	} else if( strcasecmp(parameter, "LABEL") == 0 ) {
	  if( current_cond_label[0] ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: previous COND label '%s' hasn't been finished with COND_ELSE!", line, current_cond_label);
#endif
	    current_cond_label[0] = 0;
	  }

	  char *label;
	  if( !(label = strtok_r(NULL, separators, &brkt)) ) {
	    ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: empty LABEL statement: %s", line, line_buffer);
#endif
	  } else {
	    char *str;
	    if( !(str = getQuotedString(&brkt)) ) {
	      //++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: missing string for LABEL %s!", line, label);
#endif
	    }

	    { // else {
	      int label_len = strlen(label);
	      if( label_len > 8 ) {
		++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: 'EVENT %s': label too long, should be 8 chars maximum!", line, label);
#endif
	      } else {
		int str_len = strlen(str);
		int len = 1 + 1 + 1 + label_len + 1 + str_len+1;
		if( len >= 256 ) {
		  ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: LABEL string length is greater than %d characters!", line, 256-1-1-1-label_len-1-1);
#endif
		} else {
		  ++num_labels;

#if DEBUG_VERBOSE_LEVEL >= 2
		  DEBUG_MSG("[MBNG_FILE_L:%d] writing: %d %d %s %s", line, len, MBNG_FILE_L_ITEM_UNCONDITIONAL, label, str);
#endif
		  s32 write_status;
		  if( (write_status=FILE_WriteByte(len)) < 0 ||
		      (write_status=FILE_WriteByte(MBNG_FILE_L_ITEM_UNCONDITIONAL)) < 0 ||
		      (write_status=FILE_WriteByte(label_len)) < 0 ||
		      (write_status=FILE_WriteBuffer((u8 *)label, label_len)) < 0 ||
		      (write_status=FILE_WriteByte(str_len)) < 0 ||
		      (write_status=FILE_WriteBuffer((u8 *)str, str_len+1)) < 0 // always store string terminator as well
		      ) {
		    ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		    DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: failed while writing %s, status=%d!\n", line, filepath, write_status);
#endif
		  }
		}
	      }
	    }
	  }

	} else if( strcasecmp(parameter, "COND_LABEL") == 0 ) {
	  if( current_cond_label[0] ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: previous COND label '%s' hasn't been finished with COND_ELSE!", line, current_cond_label);
#endif
	    current_cond_label[0] = 0;
	  }

	  char *label;
	  if( !(label = strtok_r(NULL, separators, &brkt)) ) {
	    ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: empty COND_LABEL statement: %s", line, line_buffer);
#endif
	  } else {
	    int label_len = strlen(label);
	    if( label_len > 8 ) {
	      ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: 'COND_LABEL %s': label too long, should be 8 chars maximum!", line, label);
#endif
	    } else {
	      int len = 1 + 1 + 1 + label_len + 1;
	      {
		++num_labels;
		memcpy(current_cond_label, label, 8);
		current_cond_label[8] = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[MBNG_FILE_L:%d] writing: %d %d %s %s", line, len, MBNG_FILE_L_ITEM_CONDITIONAL, label, current_cond_label);
#endif
		s32 write_status;
		if( (write_status=FILE_WriteByte(len)) < 0 ||
		    (write_status=FILE_WriteByte(MBNG_FILE_L_ITEM_CONDITIONAL)) < 0 ||
		    (write_status=FILE_WriteByte(label_len)) < 0 ||
		    (write_status=FILE_WriteBuffer((u8 *)label, label_len)) < 0 ||
		    (write_status=FILE_WriteByte(0) < 0 ) < 0 // no string
		    ) {
		  ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: failed while writing %s, status=%d!\n", line, filepath, write_status);
#endif
		}
	      }
	    }
	  }
	} else if( strcasecmp(parameter, "COND_ELSE") == 0 ) {
	  if( !current_cond_label[0] ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: found COND_ELSE without previous COND_LABEL statement!", line);
#endif
	    current_cond_label[0] = 0;
	  }

	  char *str;
	  if( !(str = getQuotedString(&brkt)) ) {
	    //++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: missing string for COND_ELSE condition of COND_LABEL %s!", line, current_cond_label);
#endif
	  }

	  { // else {
	    int str_len = strlen(str);
	    int len = 1 + 1 + 1 + 1 + str_len+1;
	    if( len >= 256 ) {
	      ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: COND_ELSE string length is greater than %d characters!", line, 256-1-1-1-1-1);
#endif
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	      DEBUG_MSG("[MBNG_FILE_L:%d] writing: %d %d %s", line, len, MBNG_FILE_L_ITEM_COND_ELSE, str);
#endif
	      s32 write_status;
	      if( (write_status=FILE_WriteByte(len)) < 0 ||
		  (write_status=FILE_WriteByte(MBNG_FILE_L_ITEM_COND_ELSE)) < 0 ||
		  (write_status=FILE_WriteByte(0)) < 0 || // no label
		  (write_status=FILE_WriteByte(str_len)) < 0 ||
		  (write_status=FILE_WriteBuffer((u8 *)str, str_len+1)) < 0 // always store string terminator as well
		  ) {
		++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: failed while writing %s, status=%d!\n", line, filepath, write_status);
#endif
	      }
	    }
	  }
	  current_cond_label[0] = 0;
	} else if( strcasecmp(parameter, "COND") == 0 ) {
	  if( !current_cond_label[0] ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: found COND without previous COND_LABEL statement!", line);
#endif
	    current_cond_label[0] = 0;
	  }

	  char *condition;
	  if( !(condition = strtok_r(NULL, separators, &brkt)) ) {
	    ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: empty COND statement of COND_LABEL %s: %s", line, current_cond_label, line_buffer);
#endif
	  } else {
	    int condition_len = strlen(condition);

	    u8 condition_type = MBNG_FILE_L_ITEM_COND_EQ;
	    char *condition_value = NULL;
	    if( condition[0] == '=' ) {
	      condition_type = MBNG_FILE_L_ITEM_COND_EQ;
	      condition_value = (char *)&condition[condition[1] == '=' ? 2 : 1]; // allow = and ==
	    } else if( condition[0] == '<' ) {
	      if( condition[1] == '=' ) {
		condition_type = MBNG_FILE_L_ITEM_COND_LTEQ;
		condition_value = (char *)&condition[2];
	      } else {
		condition_type = MBNG_FILE_L_ITEM_COND_LT;
		condition_value = (char *)&condition[1];
	      }
	    } else if( condition[0] == '>' ) {
	      if( condition[1] == '=' ) {
		condition_type = MBNG_FILE_L_ITEM_COND_GTEQ;
		condition_value = (char *)&condition[2];
	      } else {
		condition_type = MBNG_FILE_L_ITEM_COND_GT;
		condition_value = (char *)&condition[1];
	      }
	    }

	    if( condition_value == NULL ) {
	      ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: invalid condition '%s' for COND_LABEL %s", line, condition, current_cond_label);
#endif
	    } else if( condition_len < 2 ) {
	      ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: missing value in condition '%s' for COND_LABEL %s", line, condition, current_cond_label);
#endif
	    } else {
	      int value;
	      if( (value=get_dec(condition_value)) < 0 || value >= 65536 ) {
		++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: invalid condition value '%s' for COND_LABEL %s: %s", line, condition, current_cond_label, condition_value);
#endif
	      } else {
		char *str;
		if( !(str = getQuotedString(&brkt)) ) {
		  //++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: missing string for COND condition '%s' of COND_LABEL %s!", line, condition, current_cond_label);
#endif
		}

		{ // else {
		  int str_len = strlen(str);
		  int len = 1 + 1 + 1 + 2 + 1 + str_len+1;
		  if( len >= 256 ) {
		    ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		    DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: COND string length is greater than %d characters!", line, 256-1-1-1-2-1-1);
#endif
		  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
		    DEBUG_MSG("[MBNG_FILE_L:%d] writing: %d %d %s", line, len, condition_type, str);
#endif
		    s32 write_status;
		    if( (write_status=FILE_WriteByte(len)) < 0 ||
			(write_status=FILE_WriteByte(condition_type)) < 0 ||
			(write_status=FILE_WriteByte(2)) < 0 || // no label, but we take two bytes to embed the value for the condition
			(write_status=FILE_WriteHWord(value)) < 0 || // value
			(write_status=FILE_WriteByte(str_len)) < 0 ||
			(write_status=FILE_WriteBuffer((u8 *)str, str_len+1)) < 0 // always store string terminator as well
			) {
		      ++bin_file_errors;
#if DEBUG_VERBOSE_LEVEL >= 1
		      DEBUG_MSG("[MBNG_FILE_L:%d] ERROR: failed while writing %s, status=%d!\n", line, filepath, write_status);
#endif
		    }
		  }
		}
	      }
	    }
	  }
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  // changed error to warning, since people are sometimes confused about these messages
	  // on file format changes
	  DEBUG_MSG("[MBNG_FILE_L:%d] WARNING: unknown parameter: %s", line, line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MBNG_FILE_L:%d] ERROR no space or semicolon separator in following line: %s", line, line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // release memory from heap
  vPortFree(line_buffer);

  if( current_cond_label[0] ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] WARNING: previous COND label '%s' hasn't been finished with COND_ELSE!", current_cond_label);
#endif
    current_cond_label[0] = 0;
  }

  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_L_ERR_READ;
  }

  // close bin file
  status |= FILE_WriteClose();

  if( bin_file_errors ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] %d %s been found while generating the .BIN file - please fix %s.NGL!\n", bin_file_errors, (bin_file_errors > 2) ? "errors have" : "error has", filename);
#endif
    // delete file, so that it will be generated again...
    sprintf(filepath, "%s%s.BIN", MBNG_FILES_PATH, mbng_file_l_patch_name);
    FILE_Remove(filepath);
  } else {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] new %s.BIN file has been successfully created.\n", filename);
#endif

    // try to read file again...
    sprintf(filepath, "%s%s.BIN", MBNG_FILES_PATH, filename);
    if( parseBinFile(filepath) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[MBNG_FILE_L] ERROR: generated %s.BIN file seems to be corrupted!\n", filename);
#endif
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! help function to write data into file or send to debug terminal
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
static s32 MBNG_FILE_L_Write_Hlp(u8 write_to_file)
{
  s32 status = 0;
  char line_buffer[128];

#define FLUSH_BUFFER if( !write_to_file ) { DEBUG_MSG(line_buffer); } else { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

  strcpy(line_buffer, "# Standard label for buttons:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_btn  \"Button #%3i %3d%b\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for LEDs:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_led  \"LED #%3i       %b\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for rotary encoders:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_enc  \"ENC #%3i    %3d%B\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for pots connected to AIN pins:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_ain  \"AIN #%3i    %3d%B\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for pots connected to AINSER module:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_aser \"AINSER #%3i %3d%B\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for motorfaders:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_mf   \"MF #%3i     %3d%B\"\n\n");
  FLUSH_BUFFER;

  strcpy(line_buffer, "# Standard label for CV Channels:\n");
  FLUSH_BUFFER;
  strcpy(line_buffer, "LABEL std_cv   \"CV #%3i     %3d%B\"\n\n");
  FLUSH_BUFFER;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! writes data into label file
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Write(char *filename)
{
  mbng_file_l_info_t *info = &mbng_file_l_info;

  // store current file name in global variable for UI
  memcpy(mbng_file_l_patch_name, filename, MBNG_FILE_L_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGL", MBNG_FILES_PATH, mbng_file_l_patch_name);

  // ensure that existing file won't be overwritten
  if( FILE_FileExists(filepath) >= 1 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] The file '%s' already exists - won't create a new one!\n", filepath);
#endif
    return -1;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_L] Open label file '%s' for writing\n", filepath);
#endif

  s32 status = 0;
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_L] Failed to open/create label file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  // write file
  status |= MBNG_FILE_L_Write_Hlp(1);

  // close file
  status |= FILE_WriteClose();

  // read file (will create a new .bin file if required)
  return MBNG_FILE_L_Read(filename);
}

/////////////////////////////////////////////////////////////////////////////
//! sends label data to debug terminal
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_L_Debug(void)
{
  return MBNG_FILE_L_Write_Hlp(0); // send to debug terminal
}



/////////////////////////////////////////////////////////////////////////////
//! \returns the content of a label for a given value
//! SDCARD mutex handled automatically if SD Card really has to be accessed
/////////////////////////////////////////////////////////////////////////////
const char *MBNG_FILE_L_GetLabel(char *label, u16 value)
{
  if( mbng_file_l_info.valid ) {
    u8 *label_buffer = mbng_file_l_info.label_buffer;

    if( label_buffer[0] && label_buffer[1] == MBNG_FILE_L_ITEM_UNCONDITIONAL ) {
      u8 len = label_buffer[2];
      char *buffered_label = (char *)&label_buffer[3];

      if( strncmp(buffered_label, label, 8) == 0 )
	return (char *)&label_buffer[3 + len];
    }

    // check if label exists
    u32 i;
    u32 num_labels = mbng_file_l_info.num_labels;
    for(i=0; i<num_labels; ++i) {
      if( strncmp(mbng_file_l_info.labels[i], label, 8) == 0 ) {

	MUTEX_SDCARD_TAKE;
	s32 status;
	u32 pos = mbng_file_l_info.label_pos[i];
	if( (status=FILE_ReadReOpen(&mbng_file_l_info.bin_file)) < 0 ||
	    (status=FILE_ReadSeek(pos)) < 0 ||
	    (status=FILE_ReadByte(&label_buffer[0])) < 0 ||
	    label_buffer[0] == 0 ||
	    (status=FILE_ReadBuffer((u8 *)&label_buffer[1], label_buffer[0]-1)) < 0
	    ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	  DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while reading label %s in .BIN file\n", label);
#endif
	  label_buffer[0] = 0; // invalidate
	}
	FILE_ReadClose(&mbng_file_l_info.bin_file);
	MUTEX_SDCARD_GIVE;

	if( label_buffer[0] ) {
	  u8 label_type = label_buffer[1];

	  if( label_type == MBNG_FILE_L_ITEM_CONDITIONAL ) {
	    // search for matching condition
	    MUTEX_SDCARD_TAKE;
	    u8 condition_block = 1;
	    u8 condition_matched = 0;

	    if( (status=FILE_ReadReOpen(&mbng_file_l_info.bin_file)) >= 0 ) {
	      while( condition_block && !condition_matched ) {
		pos += label_buffer[0];
		if( (status=FILE_ReadSeek(pos)) < 0 ||
		    (status=FILE_ReadByte(&label_buffer[0])) < 0 ||
		    label_buffer[0] == 0 ||
		    (status=FILE_ReadBuffer((u8 *)&label_buffer[1], label_buffer[0]-1)) < 0
		    ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		  DEBUG_MSG("[MBNG_FILE_L] ERROR: failed while reading label %s in .BIN file\n", label);
#endif
		  break;
		}

		u8 label_type = label_buffer[1];
		s16 check_value = label_buffer[3] | ((u16)label_buffer[4] << 8);

		switch( label_type ) {
		case MBNG_FILE_L_ITEM_COND_EQ:   if( value == check_value ) { condition_matched = 1; } break;
		case MBNG_FILE_L_ITEM_COND_LT:   if( value <  check_value ) { condition_matched = 1; } break;
		case MBNG_FILE_L_ITEM_COND_LTEQ: if( value <= check_value ) { condition_matched = 1; } break;
		case MBNG_FILE_L_ITEM_COND_GT:   if( value >  check_value ) { condition_matched = 1; } break;
		case MBNG_FILE_L_ITEM_COND_GTEQ: if( value >= check_value ) { condition_matched = 1; } break;
		case MBNG_FILE_L_ITEM_COND_ELSE: condition_matched = 1; break;
		default:
		  condition_block = 0; // no condition matched, we already read the next label...
		}
	      }
	    }

	    FILE_ReadClose(&mbng_file_l_info.bin_file);
	    MUTEX_SDCARD_GIVE;

	    if( !condition_block || !condition_matched ) {
	      label_buffer[0] = 0; // invalidate buffer
	    }
	  }

	  if( label_buffer[0] ) {
	    char *label_str = (char *)&label_buffer[3 + label_buffer[2] + 1];
#if DEBUG_VERBOSE_LEVEL >= 2
	    DEBUG_MSG("[MBNG_FILE_L] label %s read into buffer: '%s'\n", label, label_str);
#endif
	    if( label_str[0] )
	      return label_str;
	  }
	}
      }
    }
  }

  // if no labels available (e.g. no SD Card connected, or std_* entries removed),
  // return at least the std_* labels used by default events that have been created in mbng_event.c
  if( strcmp(label, "std_btn") == 0 )
    return "Button #%3i %3d%b";
  if( strcmp(label, "std_led") == 0 )
    return "LED #%3i       %b";
  if( strcmp(label, "std_enc") == 0 )
    return "ENC #%3i    %3d%B";
  if( strcmp(label, "std_aser") == 0 )
    return "AINSER #%3i %3d%B";
  if( strcmp(label, "std_ain") == 0 )
    return "AIN #%3i    %3d%B";
  if( strcmp(label, "std_mf") == 0 )
    return "MF #%3i     %3d%B";
  if( strcmp(label, "std_cv") == 0 )
    return "CV #%3i     %3d%B";

  return NULL; // label not found
}


//! \}
