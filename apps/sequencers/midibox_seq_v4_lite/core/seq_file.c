// $Id$
/*
 * File access functions
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

#include <ff.h>
#include <diskio.h>
#include <string.h>

#include "tasks.h"

#include "seq_ui.h"
#include "seq_core.h"

#include "file.h"
#include "seq_file.h"

#include "seq_file_b.h"
#include "seq_file_g.h"
#include "seq_file_gc.h"
#include "seq_file_t.h"
#include "seq_file_hw.h"

#include "seq_pattern.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

// Note: verbose level 1 is default - it prints error messages
// and useful info messages during backups
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// name/directory of session
char seq_file_session_name[13];
char seq_file_new_session_name[13];

// contains the backup directory during a backup
// used by SEQ_UI to print a message on screen during a backup is created
char *seq_file_backup_notification;

// for percentage display
u8 seq_file_backup_percentage;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Init(u32 mode)
{
  s32 status = 0;

  strcpy(seq_file_session_name, "DEFAULT");
  seq_file_new_session_name[0] = 0; // invalidate
  
  seq_file_backup_notification = NULL;
  file_copy_percentage = 0;
  seq_file_backup_percentage = 0;

  status |= FILE_Init(0);
  status |= SEQ_FILE_HW_Init(0); // hardware config file access
  status |= SEQ_FILE_GC_Init(0); // global config file access
  status |= SEQ_FILE_G_Init(0); // groove file access
  status |= SEQ_FILE_B_Init(0); // pattern file access
  status |= SEQ_FILE_T_Init(0); // track preset file access

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Loads all files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_LoadAllFiles(u8 including_hw)
{
  s32 status = 0;

  if( including_hw ) {
    status |= SEQ_FILE_HW_Load();

    // ignore status if global setup file doesn't exist
    SEQ_FILE_GC_Load();
  }

  status |= SEQ_FILE_B_LoadAllBanks(seq_file_session_name);
  status |= SEQ_FILE_G_Load(seq_file_session_name);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// invalidate all file infos
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_UnloadAllFiles(void)
{
  s32 status = 0;
  status |= SEQ_FILE_B_UnloadAllBanks();
  status |= SEQ_FILE_G_Unload();
  status |= SEQ_FILE_GC_Unload();
  status |= SEQ_FILE_HW_Unload();

  // invalidate session
  strcpy(seq_file_session_name, "DEFAULT");
  seq_file_new_session_name[0] = 0; // invalidate

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores the current name of the session in a file (/SESSIONS/LAST_ONE.V4T), so 
// that it can be restored after startup
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_StoreSessionName(void)
{
  s32 status;
  char filepath[30];

  sprintf(filepath, "%s/LAST_ONE.V4T", SEQ_FILE_SESSION_PATH);
  status=FILE_WriteOpen(filepath, 1);
  if( status >= 0 ) {
    status = FILE_WriteBuffer((u8 *)seq_file_session_name, strlen(seq_file_session_name));
    if( status >= 0 )
      status = FILE_WriteByte('\n');
    FILE_WriteClose();
  }

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_StoreSessionName] ERROR: failed to store last session name (status: %d)\n", status);
#endif
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Restores the last session from the file /SESSIONS/LAST_ONE.V4T
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_LoadSessionName(void)
{
  s32 status;
  char filepath[30];
  file_t file;

  sprintf(filepath, "%s/LAST_ONE.V4T", SEQ_FILE_SESSION_PATH);
  if( (status=FILE_ReadOpen(&file, filepath)) >= 0 ) {
    char linebuffer[20];
    status = FILE_ReadLine((u8 *)&linebuffer, 20);
    if( status < 0 || strlen(linebuffer) > 8 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_LoadSessionName] ERROR: invalid session name '%s'\n", linebuffer);
#endif
      status = FILE_ERR_INVALID_SESSION_NAME;
    } else {
      // take over session name
      strcpy(seq_file_session_name, linebuffer);
    }

    FILE_ReadClose(&file);
  }
  return status;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if it is required to format any file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_FormattingRequired(void)
{
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank)
    if( !SEQ_FILE_B_NumPatterns(bank) )
      return 1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This function formats Bank/Mixer/Song files
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_Format(void)
{
  s32 status = 0;

  if( !FILE_VolumeAvailable() ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( seq_file_new_session_name[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_Format] ERROR: no new session specified!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  // switch to new session name, store old name for the case that we have to switch back
  char prev_session_name[13];
  strcpy(prev_session_name, seq_file_session_name);
  strcpy(seq_file_session_name, seq_file_new_session_name); 


#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SEQ_FILE_Format] Creating Session %s (previous was %s)\n", seq_file_session_name, prev_session_name);
#endif

  u8 num_operations = SEQ_FILE_B_NUM_BANKS + 1 + 1 + 1 + 1;
  char filename_buffer[30];
  seq_file_backup_notification = filename_buffer;

  file_copy_percentage = 0; // for percentage display

  // create banks
  u8 bank;
  for(bank=0; bank<SEQ_FILE_B_NUM_BANKS; ++bank) {
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)bank) / num_operations);
    sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_B%d.V4T", SEQ_FILE_SESSION_PATH, seq_file_new_session_name, bank+1);

    if( (status=SEQ_FILE_B_Create(seq_file_session_name, bank)) < 0 )
      goto SEQ_FILE_Format_failed;

    // fill patterns with useful data
    int pattern;
    int num_patterns = SEQ_FILE_B_NumPatterns(bank);
    for(pattern=0; pattern<num_patterns; ++pattern) {
      file_copy_percentage = (u8)(((u32)100 * (u32)pattern) / num_patterns); // for percentage display
      u8 group = bank % SEQ_CORE_NUM_GROUPS; // note: bank selects source group

      if( (status=SEQ_FILE_B_PatternWrite(seq_file_session_name, bank, pattern, group, 0)) < 0 )
	goto SEQ_FILE_Format_failed;
    }

    // open bank
    if( (status=SEQ_FILE_B_Open(seq_file_session_name, bank)) < 0 )
      goto SEQ_FILE_Format_failed;
  }


  // create grooves
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+2)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_G.V4T", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  if( (status=SEQ_FILE_G_Write(seq_file_session_name)) < 0 )
    goto SEQ_FILE_Format_failed;

SEQ_FILE_Format_failed:
  if( status >= 0 ) {
    // we were successfull
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] Session %s created successfully!\n", seq_file_session_name);
#endif

    // reload setup
    SEQ_FILE_LoadAllFiles(0); // excluding HW info
  } else {
    // we were not successfull!
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_Format] Session %s failed with status %d!\n", seq_file_session_name, status);
#endif

    // switch back to old session name
    strcpy(seq_file_session_name, prev_session_name); 
  }

  // in any case invalidate new session name
  seq_file_new_session_name[0] = 0;

  // no need to check for existing config file (will be created once config data is stored)
  return status;
}
