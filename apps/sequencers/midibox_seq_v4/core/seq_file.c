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
#include "seq_file_c.h"
#include "seq_file_gc.h"
#include "seq_file_t.h"
#include "seq_file_hw.h"
#include "seq_file_s.h"
#include "seq_file_m.h"
#include "seq_file_bm.h"

#include "seq_mixer.h"
#include "seq_pattern.h"
#include "seq_song.h"


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
  status |= SEQ_FILE_C_Init(0); // config file access
  status |= SEQ_FILE_G_Init(0); // groove file access
  status |= SEQ_FILE_B_Init(0); // pattern file access
  status |= SEQ_FILE_T_Init(0); // track preset file access
  status |= SEQ_FILE_M_Init(0); // mixer file access
  status |= SEQ_FILE_S_Init(0); // song file access
  status |= SEQ_FILE_BM_Init(0); // bookmarks file access

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
  status |= SEQ_FILE_M_LoadAllBanks(seq_file_session_name);
  status |= SEQ_FILE_S_LoadAllBanks(seq_file_session_name);
  status |= SEQ_FILE_G_Load(seq_file_session_name);

  // ignore status if bookmark file doesn't exist
  SEQ_FILE_BM_Load(seq_file_session_name, 1); // global
  SEQ_FILE_BM_Load(seq_file_session_name, 0); // session

  if( SEQ_FILE_C_Load(seq_file_session_name) >= 0 ) {
    // change mixer map to the one stored in MBSEQ_C.V4
    SEQ_MIXER_Load(SEQ_MIXER_NumGet());

    // change patterns to the ones stored in MBSEQ_C.V4
    int group;
    for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
      SEQ_PATTERN_Change(group, seq_pattern[group], 0);

    // change song to the one stored in MBSEQ_C.V4
    SEQ_SONG_Load(SEQ_SONG_NumGet());
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// invalidate all file infos
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_UnloadAllFiles(void)
{
  s32 status = 0;
  status |= SEQ_FILE_B_UnloadAllBanks();
  status |= SEQ_FILE_M_UnloadAllBanks();
  status |= SEQ_FILE_S_UnloadAllBanks();
  status |= SEQ_FILE_G_Unload();
  status |= SEQ_FILE_C_Unload();
  status |= SEQ_FILE_GC_Unload();
  status |= SEQ_FILE_BM_Unload(0);
  status |= SEQ_FILE_BM_Unload(1);
  status |= SEQ_FILE_HW_Unload();

  // invalidate session
  strcpy(seq_file_session_name, "DEFAULT");
  seq_file_new_session_name[0] = 0; // invalidate

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores the current name of the session in a file (/SESSIONS/LAST_ONE.V4), so 
// that it can be restored after startup
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_StoreSessionName(void)
{
  s32 status;
  char filepath[30];

  sprintf(filepath, "%s/LAST_ONE.V4", SEQ_FILE_SESSION_PATH);
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
// Restores the last session from the file /SESSIONS/LAST_ONE.V4
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_LoadSessionName(void)
{
  s32 status;
  char filepath[30];
  file_t file;

  sprintf(filepath, "%s/LAST_ONE.V4", SEQ_FILE_SESSION_PATH);
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

  if( !SEQ_FILE_M_NumMaps() )
    return 1;

  if( !SEQ_FILE_S_NumSongs() )
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
    sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_B%d.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name, bank+1);

    SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)

    if( (status=SEQ_FILE_B_Create(seq_file_session_name, bank)) < 0 )
      goto SEQ_FILE_Format_failed;

    SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)

    // fill patterns with useful data
    int pattern;
    int num_patterns = SEQ_FILE_B_NumPatterns(bank);
    for(pattern=0; pattern<num_patterns; ++pattern) {
      file_copy_percentage = (u8)(((u32)100 * (u32)pattern) / num_patterns); // for percentage display
      u8 group = bank % SEQ_CORE_NUM_GROUPS; // note: bank selects source group

      SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)

      if( (status=SEQ_FILE_B_PatternWrite(seq_file_session_name, bank, pattern, group, 0)) < 0 )
	goto SEQ_FILE_Format_failed;
    }

    // open bank
    if( (status=SEQ_FILE_B_Open(seq_file_session_name, bank)) < 0 )
      goto SEQ_FILE_Format_failed;
  }


  // create mixer maps
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+0)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_M.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)
  if( (status=SEQ_FILE_M_Create(seq_file_session_name)) >= 0 ) {
    int map;
    int num_maps = SEQ_FILE_M_NumMaps();
    for(map=0; map<num_maps; ++map) {
      file_copy_percentage = (u8)(((u32)100 * (u32)map) / num_maps); // for percentage display
      if( (status = SEQ_FILE_M_MapWrite(seq_file_session_name, map, 0)) < 0 )
	goto SEQ_FILE_Format_failed;
    }

    if( (status=SEQ_FILE_M_Open(seq_file_session_name)) < 0 )
      goto SEQ_FILE_Format_failed;
  }

  // create song
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+1)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_S.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)
  if( (status=SEQ_FILE_S_Create(seq_file_session_name)) >= 0 ) {
    int song;
    int num_songs = SEQ_FILE_S_NumSongs();
    for(song=0; song<num_songs; ++song) {
      file_copy_percentage = (u8)(((u32)100 * (u32)song) / num_songs); // for percentage display
      if( (status = SEQ_FILE_S_SongWrite(seq_file_session_name, song, 0)) )
	goto SEQ_FILE_Format_failed;
    }

    if( (status=SEQ_FILE_S_Open(seq_file_session_name)) < 0 )
      goto SEQ_FILE_Format_failed;
  }


  // create grooves
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+2)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_G.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)
  if( (status=SEQ_FILE_G_Write(seq_file_session_name)) < 0 )
    goto SEQ_FILE_Format_failed;


  // create config
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+3)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_C.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)
  if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
    goto SEQ_FILE_Format_failed;


  // create bookmarks
  seq_file_backup_percentage = (u8)(((u32)100 * (u32)(SEQ_FILE_B_NUM_BANKS+3)) / num_operations);
  sprintf(seq_file_backup_notification, "%s/%s/MBSEQ_BM.V4", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);
  SEQ_UI_LCD_Handler(); // update LCD (required, since file and LCD task have been merged)
  if( (status=SEQ_FILE_BM_Write(seq_file_session_name, 0)) < 0 )
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


/////////////////////////////////////////////////////////////////////////////
// This function creates a backup of all MBSEQ files
// Source: seq_file_session_name
// Destination: seq_file_new_session_name
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_FILE_CreateBackup(void)
{
  s32 status = 0;

  if( !FILE_VolumeAvailable() ) {
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: volume doesn't exist!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  if( seq_file_new_session_name[0] == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[SEQ_FILE_CreateBackup] ERROR: no new session specified!\n");
#endif
    return FILE_ERR_NO_VOLUME;
  }

  char src_path[40];
  sprintf(src_path, "%s/%s", SEQ_FILE_SESSION_PATH, seq_file_session_name);
  char dst_path[40];
  sprintf(dst_path, "%s/%s", SEQ_FILE_SESSION_PATH, seq_file_new_session_name);

  char src_file[50];
  char dst_file[50];
  // We assume that session directory already has been created in seq_ui_menu.c

#define COPY_FILE_MACRO(name) if( status >= 0 ) { \
    sprintf(src_file, "%s/%s", src_path, name);   \
    sprintf(dst_file, "%s/%s", dst_path, name);   \
    seq_file_backup_notification = dst_file;      \
    SEQ_UI_LCD_Handler();                         \
    status = FILE_Copy(src_file, dst_file); \
    if( status == FILE_ERR_COPY_NO_FILE ) status = 0;   \
    ++seq_file_backup_file;				    \
    seq_file_backup_percentage = (u8)(((u32)100 * (u32)seq_file_backup_file) / seq_file_backup_files); \
  }

  // this approach saves some stack - we don't want to allocate more memory by using
  // temporary variables to create src_file and dst_file from an array...
  seq_file_backup_percentage = 0;
  u8 seq_file_backup_files = SEQ_FILE_B_NUM_BANKS+5; // for percentage display
  u8 seq_file_backup_file = 0;
  COPY_FILE_MACRO("MBSEQ_B1.V4");
  COPY_FILE_MACRO("MBSEQ_B2.V4");
  COPY_FILE_MACRO("MBSEQ_B3.V4");
  COPY_FILE_MACRO("MBSEQ_B4.V4");
  COPY_FILE_MACRO("MBSEQ_G.V4");
  COPY_FILE_MACRO("MBSEQ_M.V4");
  COPY_FILE_MACRO("MBSEQ_C.V4");
  COPY_FILE_MACRO("MBSEQ_BM.V4");
  COPY_FILE_MACRO("MBSEQ_S.V4"); // important: should be the last file to notify that backup is complete!

  // stop printing the special message
  seq_file_backup_notification = NULL;

  if( status >= 0 ) {
    // we were successfull!
    status = 1;
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] backup of %s passed, new name: %s\n", seq_file_session_name, seq_file_new_session_name);
#endif

    // take over session name
    strcpy(seq_file_session_name, seq_file_new_session_name);

    // load files
    status = SEQ_FILE_LoadAllFiles(0); // excluding HW config

#if DEBUG_VERBOSE_LEVEL >= 2
    if( status < 0 )
      DEBUG_MSG("[SEQ_FILE_CreateBackup] failed during LoadAllFiles at %s, status %d\n", seq_file_new_session_name, status);
#endif

  } else { // found errors!
#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_FILE_CreateBackup] backup of %s failed, status %d\n", seq_file_new_session_name, status);
#endif
    status = FILE_ERR_COPY;
  }

  // in any case invalidate new session name
  seq_file_new_session_name[0] = 0;

  return status;
}
