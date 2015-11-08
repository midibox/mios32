// $Id$
//! \defgroup MBNG_FILE_K
//! Keyboard calibration data
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
#include "app.h"

#include <string.h>

#include <keyboard.h>

#include "file.h"
#include "mbng_file.h"
#include "mbng_file_k.h"


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


/////////////////////////////////////////////////////////////////////////////
//! Local types
/////////////////////////////////////////////////////////////////////////////

// file informations stored in RAM
typedef struct {
  unsigned valid: 1;   // file is accessible
} mbng_file_k_info_t;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static mbng_file_k_info_t mbng_file_k_info;


/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_K_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Loads keyboard file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been connected
//! \param filename the filename which should be read
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Load(char *filename)
{
  s32 error;
  error = MBNG_FILE_K_Read(filename);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_S] Tried to open keyboard file %s, status: %d\n", filename, error);
#endif

  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! Unloads keyboard file
//! Called from MBNG_FILE_CheckSDCard() when the SD card has been disconnected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Unload(void)
{
  mbng_file_k_info.valid = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \returns 1 if current keyboard file valid
//! \returns 0 if current keyboard file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Valid(void)
{
  return mbng_file_k_info.valid;
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
//! Reads the keyboard calibration data file content
//! \param filename the filename which should be read
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Read(char *filename)
{
  s32 status = 0;
  mbng_file_k_info_t *info = &mbng_file_k_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGK", MBNG_FILES_PATH, filename);
  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[MBNG_FILE_K] failed to open file, status: %d\n", status);
#endif
    return status;
  }

  // read keyboard definitions
  u32 line_buffer_size = 1024;
  char *line_buffer = pvPortMalloc(line_buffer_size);
  u32 line_buffer_len = 0;
  if( !line_buffer ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_K] FATAL: out of heap memory!\n");
#endif
    return -1;
  }

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
	} else if( strcasecmp(parameter, "CAL") == 0 ) {
	  int kb, key, delay;

	  if( !(parameter = strtok_r(NULL, separators, &brkt)) ||
	      (kb=get_dec(parameter)) < 0 ||
	      !(parameter = strtok_r(NULL, separators, &brkt)) ||
	      (key=get_dec(parameter)) < 0 ||
	      !(parameter = strtok_r(NULL, separators, &brkt)) ||
	      (delay=get_dec(parameter)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_K:%d] ERROR: invalid syntax for CAL statement: %s", line, line_buffer);
#endif
	  } else {
	    if( kb < 1 || kb > KEYBOARD_NUM ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_K:%d] ERROR: invalid <kb> number, expect 1..%d", line, KEYBOARD_NUM);
#endif
	    } else if( key >= KEYBOARD_MAX_KEYS ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_K:%d] ERROR: invalid <key> number, expect 0..%d", line, KEYBOARD_MAX_KEYS-1);
#endif
	    } else if( delay >= 65536 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_K:%d] ERROR: invalid <delay> number, expect 0..65535", line);
#endif
	    } else {
	      keyboard_config[kb-1].delay_key[key] = delay;
	    }
	  }
	  
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  // changed error to warning, since people are sometimes confused about these messages
	  // on file format changes
	  DEBUG_MSG("[MBNG_FILE_K:%d] WARNING: unknown parameter: %s", line, line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MBNG_FILE_K:%d] ERROR no space or semicolon separator in following line: %s", line, line_buffer);
#endif
      }
    }

  } while( status >= 1 );

  // release memory from heap
  vPortFree(line_buffer);


  // close file
  status |= FILE_ReadClose(&file);

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_K] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_K_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 1; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! writes data into keyboard calibration file
//! \param filename the filename which should be written
//! \returns < 0 on errors (error codes are documented in seq_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_K_Write(char *filename)
{
  s32 status = 0;
  mbng_file_k_info_t *info = &mbng_file_k_info;

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGK", MBNG_FILES_PATH, filename);

  // write data
  if( (status=FILE_WriteOpen(filepath, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_K] Failed to create keyboard file, status: %d\n", status);
#endif
    FILE_WriteClose(); // important to free memory given by malloc
    info->valid = 0;
    return status;
  }

  {
    char line_buffer[256];
#define FLUSH_BUFFER { status |= FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer)); }

    sprintf(line_buffer, "# Calibration Data\n");
    FLUSH_BUFFER;

    sprintf(line_buffer, "# SYNTAX: CALI <kb> <key> <delay>\n");
    FLUSH_BUFFER;

    {
      int kb;
      keyboard_config_t *kc = (keyboard_config_t *)&keyboard_config[0];
      for(kb=0; kb<KEYBOARD_NUM; ++kb, ++kc) {
	int i;

	for(i=0; i<KEYBOARD_MAX_KEYS; ++i) {
	  sprintf(line_buffer, "CAL %d %3d %d\n", kb+1, i, kc->delay_key[i]);
	  FLUSH_BUFFER;
	}
      }
    }
  }

  FILE_WriteClose(); // important to free memory given by malloc

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_K] ERROR: failed while writing %s!\n", filepath);
#endif
    return status;
  }

  return 0; // no error
}

//! \}
