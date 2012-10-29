// $Id$
/*
 * MIDI File Access Routines
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
#include "tasks.h"

#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <string.h>

#include "file.h"
#include "mid_file.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIDI
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// in which subdirectory of the SD card are the .mid files located?
// use "" for root
// use "<dir>/" for a subdirectory in root
// use "<dir>/<subdir>/" to reach a subdirectory in <dir>, etc..

#define MID_FILES_PATH ""
//#define MID_FILES_PATH "MyMidi/"

// max path length
#define MAX_PATH 100


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the file position and length
static u32 midifile_pos;
static u32 midifile_len;
static char ui_midifile_name[MAX_PATH];

// MIDI file references
static file_t midifile_fi;

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_Init(u32 mode)
{
  // initial midifile name and size
  ui_midifile_name[0] = 0;
  midifile_len = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the filename of the .mid file for the user interface
// Contains an error message if file wasn't loaded correctly
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_UI_NameGet(void)
{
  return ui_midifile_name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the .mid file next to the given filename
// returns < 0 on errors
// returns 1 if a new file has been found, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_FindNext(char *filename, char *next_file)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = FILE_FindNextFile(MID_FILES_PATH,
			     filename,
			     "MID",
			     next_file);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    MIDIO_FILE_StatusMsgSet("SDCard Err!");
  } else if( status == 0 ) {
    MIDIO_FILE_StatusMsgSet("No .MID File");
  } else {
    MIDIO_FILE_StatusMsgSet(NULL);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the .mid file previous to the given filename
// returns < 0 on errors
// returns 1 if a new file has been found, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_FindPrev(char *filename, char *prev_file)
{
  s32 status;

  MUTEX_SDCARD_TAKE;
  status = FILE_FindPreviousFile(MID_FILES_PATH,
				 filename,
				 "MID",
				 prev_file);
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
    MIDIO_FILE_StatusMsgSet("SDCard Err!");
  } else if( status == 0 ) {
    MIDIO_FILE_StatusMsgSet("No .MID File");
  } else {
    MIDIO_FILE_StatusMsgSet(NULL);
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Open a .mid file with given filename
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_open(char *filename)
{
  char filepath[MAX_PATH];
  strncpy(filepath, MID_FILES_PATH, MAX_PATH);
  strncat(filepath, filename, MAX_PATH);
  
  MUTEX_SDCARD_TAKE;
  s32 status = FILE_ReadOpen(&midifile_fi, filepath);
  FILE_ReadClose(&midifile_fi); // close again - file will be reopened by read handler
  MUTEX_SDCARD_GIVE;

  if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[MID_FILE] failed to open file, status: %d\n", status);
#endif
    ui_midifile_name[0] = 0; // disable file
  } else {

    // got it
    midifile_pos = 0;
    midifile_len = midifile_fi.fsize;

    strncpy(ui_midifile_name, filepath, MAX_PATH);
    ui_midifile_name[MAX_PATH-1] = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[SEQ_MIDFILE] opened '%s' of length %u\n", filepath, midifile_len);
#endif
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 MID_FILE_read(void *buffer, u32 len)
{
  s32 status;

  if( !ui_midifile_name[0] )
    return FILE_ERR_NO_FILE;

  MUTEX_SDCARD_TAKE;
  if( (status=FILE_ReadReOpen(&midifile_fi)) >= 0 ) {
    status = FILE_ReadBuffer(buffer, len);
    FILE_ReadClose(&midifile_fi);
  }
  MUTEX_SDCARD_GIVE;

  return (status >= 0) ? len : 0;
}


/////////////////////////////////////////////////////////////////////////////
// returns 1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_eof(void)
{
  if( midifile_pos >= midifile_len )
    return 1; // end of file reached

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// sets file pointer to a specific position
// returns -1 if end of file reached
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_seek(u32 pos)
{
  s32 status;

  if( !ui_midifile_name[0] )
    return -1; // end of file reached

  midifile_pos = pos;

  MUTEX_SDCARD_TAKE;

  if( midifile_pos >= midifile_len )
    status = -1; // end of file reached
  else {
    if( (status=FILE_ReadReOpen(&midifile_fi)) >= 0 ) {
      status = FILE_ReadSeek(pos);    
      FILE_ReadClose(&midifile_fi);
    }
  }

  MUTEX_SDCARD_GIVE;

  return status;
}
