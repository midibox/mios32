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
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include <string.h>

#include "mid_file.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// the file position and length
static u32 midifile_pos;
static u32 midifile_len;
static char midifile_name[13];

// including the .mid file (located in internal flash)
#include "mb_midifile_demo.inc"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_Init(u32 mode)
{
  strcpy(midifile_name, "waiting...");
  midifile_len = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the filename of the .mid file
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_NameGet(void)
{
  return midifile_name;
}


/////////////////////////////////////////////////////////////////////////////
// Returns the .mid file next to the given filename
// if filename == NULL, the first .mid file will be returned
// if returned filename == NULL, no .mid file has been found
// dest must point to a 13-byte buffer
/////////////////////////////////////////////////////////////////////////////
char *MID_FILE_FindNext(char *filename)
{
  // we only support a single file
  strcpy(midifile_name, "DEMO.MID");
  return midifile_name;
}


/////////////////////////////////////////////////////////////////////////////
// Open a .mid file with given filename
/////////////////////////////////////////////////////////////////////////////
s32 MID_FILE_open(char *filename)
{
  // only a single .mid file in flash - ignore the filename
  midifile_pos = 0;
  midifile_len = MID_FILE_LEN;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// reads <len> bytes from the .mid file into <buffer>
// returns number of read bytes
/////////////////////////////////////////////////////////////////////////////
u32 MID_FILE_read(void *buffer, u32 len)
{
  memcpy(buffer, &mid_file[midifile_pos], len);
  midifile_pos += len;
  return len;
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
  midifile_pos = pos;
  if( midifile_pos >= midifile_len )
    return -1; // end of file reached

  return 0;
}
