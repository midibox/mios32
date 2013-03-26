// $Id$
//! \defgroup MBNG_FILE_R
//! Config File access functions
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
#include <string.h>
#include <uip_task.h>

#include "tasks.h"
#include "file.h"
#include "mbng_file.h"
#include "mbng_file_r.h"
#include "mbng_event.h"
#include "mbng_lcd.h"


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
} mbng_file_r_info_t;


/////////////////////////////////////////////////////////////////////////////
//! Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//! Local variables
/////////////////////////////////////////////////////////////////////////////

static mbng_file_r_info_t mbng_file_r_info;


/////////////////////////////////////////////////////////////////////////////
//! Global variables
/////////////////////////////////////////////////////////////////////////////
char mbng_file_r_script_name[MBNG_FILE_R_FILENAME_LEN+1];
mbng_file_r_req_t mbng_file_r_req;


/////////////////////////////////////////////////////////////////////////////
//! Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Init(u32 mode)
{
  // invalidate file info
  MBNG_FILE_R_Unload();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Loads script
//! Called from MBNG_FILE_RheckSDCard() when the SD card has been connected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Load(char *filename)
{
  s32 error;
  error = MBNG_FILE_R_Read(filename, 0);
#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R] Tried to open script %s, status: %d\n", filename, error);
#endif

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! Unloads script file
//! Called from MBNG_FILE_RheckSDCard() when the SD card has been disconnected
//! \returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Unload(void)
{
  mbng_file_r_info.valid = 0;
  mbng_file_r_req.ALL = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! \Returns 1 if current script file valid
//! \Returns 0 if current script file not valid
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Valid(void)
{
  return mbng_file_r_info.valid;
}


/////////////////////////////////////////////////////////////////////////////
//! help function which removes the quotes of an argument (e.g. .csv file format)
//! can be cascaded with strtok_r
/////////////////////////////////////////////////////////////////////////////
static char *remove_quotes(char *word)
{
  if( word == NULL )
    return NULL;

  if( *word == '"' )
    ++word;

  int len = strlen(word);
  if( len && word[len-1] == '"' )
    word[len-1] = 0;

  return word;
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
//! help function which parses an id (xxx:<number>)
//! \returns > 0 if id is valid
//! \returns == 0 if id is invalid
/////////////////////////////////////////////////////////////////////////////
static mbng_event_item_id_t parseId(char *parameter, char **brkt)
{
  const char *separator_colon = ":";
  const char *separators = " \t;";
  mbng_event_item_id_t id;
  char *value_str;

  if( !(value_str = strtok_r(parameter, separator_colon, brkt)) ||
      (id=MBNG_EVENT_ItemIdFromControllerStrGet(value_str)) == MBNG_EVENT_CONTROLLER_DISABLED ) {
    return 0;
  }

  int id_lower = 0;
  if( !(value_str = strtok_r(NULL, separators, brkt)) ||
      (id_lower=get_dec(value_str)) < 1 || id_lower > 0xfff ) {
    return 0;
  }

  return id | id_lower;
}


/////////////////////////////////////////////////////////////////////////////
//! reads the config file content (again)
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_Read(char *filename, u8 section)
{
  s32 status = 0;
  mbng_file_r_info_t *info = &mbng_file_r_info;
  file_t file;

  info->valid = 0; // will be set to valid if file content has been read successfully

  // store current file name in global variable for UI
  memcpy(mbng_file_r_script_name, filename, MBNG_FILE_R_FILENAME_LEN+1);

  char filepath[MAX_PATH];
  sprintf(filepath, "%s%s.NGR", MBNG_FILES_PATH, mbng_file_r_script_name);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[MBNG_FILE_R] Open config '%s'\n", filepath);
#endif

  if( (status=FILE_ReadOpen(&file, filepath)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R] %s (optional run script) not found\n", filepath);
#endif
    return status;
  }

  // allocate 1024 bytes from heap
  u32 line_buffer_size = 1024;
  char *line_buffer = pvPortMalloc(line_buffer_size);
  u32 line_buffer_len = 0;
  if( !line_buffer ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MBNG_FILE_R] FATAL: out of heap memory!\n");
#endif
    return -1;
  }

  // read commands
  do {
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
      const char *separators = " \t;";
      char *brkt;
      char *parameter;

      if( (parameter = remove_quotes(strtok_r(line_buffer, separators, &brkt))) ) {
	
	if( *parameter == 0 || *parameter == '#' ) {
	  // ignore comments and empty lines
	} else if( strcmp(parameter, "LCD") == 0 ) {
	  char *str = brkt;
	  if( !(str=remove_quotes(str)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_R] ERROR: missing string after LCD message!\n");
#endif
	  } else {
	    // print from a dummy item
	    mbng_event_item_t item;
	    MBNG_EVENT_ItemInit(&item, MBNG_EVENT_CONTROLLER_DISABLED);
	    item.label = str;
	    MBNG_LCD_PrintItemLabel(&item);
	  }
	} else if( strcmp(parameter, "SET") == 0 ) {

	  mbng_event_item_id_t id;

	  char *id_str = brkt;
	  if( !(id=parseId(NULL, &brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_R] ERROR: missing or invalid id '%s %s'!\n", parameter, id_str);
#endif
	  } else {
	    char *value_str;
	    s32 value;
	    if( !(value_str = strtok_r(NULL, separators, &brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_R] ERROR: missing value after '%s %s:%d' command!\n", parameter, MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff);
#endif
	    } else if( (value=get_dec(value_str)) < -16384 || value >= 16383 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	      DEBUG_MSG("[MBNG_FILE_R] ERROR: invalid value in '%s %s:%d %s' command (expecting -16384..16383!\n", parameter, MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, value_str);
#endif
	    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	      DEBUG_MSG("[MBNG_FILE_R] %s:%d = %d\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, value);
#endif

	      // search for fwd item
	      mbng_event_item_t item;
	      u32 continue_ix = 0;
	      u32 num_set = 0;
	      do {
		if( MBNG_EVENT_ItemSearchById(id, &item, &continue_ix) < 0 ) {
		  break;
		} else {
		  ++num_set;

		  // notify item
		  item.value = value;
		  if( MBNG_EVENT_NotifySendValue(&item) == 2 )
		    break; // stop has been requested
		}
	      } while( continue_ix );

	      if( !num_set ) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[MBNG_FILE_R] 'SET %s:%d %d' failed - item not found!\n", MBNG_EVENT_ItemControllerStrGet(id), id & 0xfff, value);
#endif
	      }
	    }
	  }

	} else if( strcmp(parameter, "DELAY_MS") == 0 ) {
	  char *value_str;
	  s32 value;
	  if( !(value_str = strtok_r(NULL, separators, &brkt)) ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_R] ERROR: missing value after '%s' command!\n", parameter);
#endif
	  } else if( (value=get_dec(value_str)) < 0 || value > 100000 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
	    DEBUG_MSG("[MBNG_FILE_R] ERROR: invalid value in '%s %s' command (expecting 0..100000)!\n", parameter, value_str);
#endif
	  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	    DEBUG_MSG("[MBNG_FILE_R] DELAY_MS %d\n", value);
#endif
	    int i;
	    for(i=0; i<value; ++i)
	      MIOS32_DELAY_Wait_uS(1000);
	  }
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
	  // changed error to warning, since people are sometimes confused about these messages
	  // on file format changes
	  DEBUG_MSG("[MBNG_FILE_R] WARNING: unknown command: %s", line_buffer);
#endif
	}
      } else {
#if DEBUG_VERBOSE_LEVEL >= 2
	// no real error, can for example happen in .csv file
	DEBUG_MSG("[MBNG_FILE_R] ERROR no space or semicolon separator in following line: %s", line_buffer);
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
    DEBUG_MSG("[MBNG_FILE_R] ERROR while reading file, status: %d\n", status);
#endif
    return MBNG_FILE_R_ERR_READ;
  }

  // file is valid! :)
  info->valid = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! request to read the script file from run thread
//! \returns < 0 on errors (error codes are documented in mbng_file.h)
/////////////////////////////////////////////////////////////////////////////
s32 MBNG_FILE_R_ReadRequest(char *filename, u8 section)
{
  // store current file name in global variable for UI
  memcpy(mbng_file_r_script_name, filename, MBNG_FILE_R_FILENAME_LEN+1);

  // set request (and value)
  mbng_file_r_req.section = section;
  mbng_file_r_req.load = 1;

  return 0;
}

//! \}
