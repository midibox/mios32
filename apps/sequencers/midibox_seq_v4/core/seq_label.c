// $Id$
/*
 * Label Routines
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

#include "tasks.h"

#include "seq_label.h"

#include "seq_file_presets.h"


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_Init(u32 mode)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of label presets
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_NumPresets(void)
{
  return SEQ_FILE_PRESETS_TrkLabel_NumGet()+1;
}

/////////////////////////////////////////////////////////////////////////////
// This function copies a preset label (15 chars)
// Could be loaded from SD-Card later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_CopyPreset(u8 num, char *dst)
{
  const int max_chars = 15;
  s32 status = 0;

  if( num >= 1 && num <= SEQ_FILE_PRESETS_TrkLabel_NumGet() ) {
    char str[max_chars+1];

    MUTEX_SDCARD_TAKE;
    status = SEQ_FILE_PRESETS_TrkLabel_Read(num, str);
    MUTEX_SDCARD_GIVE;

    if( status > 0 ) {
      // fill with spaces
      size_t len = strlen(str);
      strncpy(dst, str, max_chars);
      int i;
      for(i=len; i<max_chars; ++i)
	dst[i] = ' ';
    }
  }

  if( status < 1 ) {
    memcpy(dst, "               ", max_chars);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of category presets
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_NumPresetsCategory(void)
{
  return SEQ_FILE_PRESETS_TrkCategory_NumGet()+1;
}

/////////////////////////////////////////////////////////////////////////////
// This function copies a preset category (5 chars) into the given array
// Could be loaded from SD-Card later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_CopyPresetCategory(u8 num, char *dst)
{
  const int max_chars = 5;
  s32 status = 0;

  if( num >= 1 && num <= SEQ_FILE_PRESETS_TrkCategory_NumGet() ) {
    char str[max_chars+1];

    MUTEX_SDCARD_TAKE;
    status = SEQ_FILE_PRESETS_TrkCategory_Read(num, str);
    MUTEX_SDCARD_GIVE;

    if( status > 0 ) {
      // fill with spaces
      size_t len = strlen(str);
      strncpy(dst, str, max_chars);
      int i;
      for(i=len; i<max_chars; ++i)
	dst[i] = ' ';
    }
  }

  if( status < 1 ) {
    memcpy(dst, "     ", max_chars);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of drum presets
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_NumPresetsDrum(void)
{
  return SEQ_FILE_PRESETS_TrkDrum_NumGet();
}

/////////////////////////////////////////////////////////////////////////////
// This function copies a preset drum label (5 chars) into the given array
// Could be loaded from SD-Card later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_CopyPresetDrum(u8 num, char *dst, u8 *note)
{
  const int max_chars = 5;
  s32 status = 0;

  if( num >= 0 && num < SEQ_FILE_PRESETS_TrkDrum_NumGet() ) {
    char str[max_chars+1];

    MUTEX_SDCARD_TAKE;
    status = SEQ_FILE_PRESETS_TrkDrum_Read(num+1, str, note, 0);
    MUTEX_SDCARD_GIVE;

    if( status > 0 ) {
      // fill with spaces
      size_t len = strlen(str);
      strncpy(dst, str, max_chars);
      int i;
      for(i=len; i<max_chars; ++i)
	dst[i] = ' ';
    }
  }

  if( status < 1 ) {
    memcpy(dst, "     ", max_chars);
  }

  return 0; // no error
}
