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

#include "seq_label.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char preset_categories[][6] = {
  "     ",
  "Synth",
  "Piano",
  "Bass ",
  "Drums",
  "Break",
  "MBSID",
  "MBFM ",
  "Ctrl"
};


static const char preset_labels[][16] = {
  "               ",
  "Vintage        ",
  "Synthline      ",
  "Bassline       ",
  "Pads           ",
  "Chords         ",
  "Lovely Arps    ",
  "Electr. Drums  ",
  "Heavy Beats    ",
  "Simple Beats   ",
  "CC Tracks      ",
  "Experiments    ",
  "Live Played    ",
  "Transposer     "
};



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
  return sizeof(preset_labels) / 16;
}

/////////////////////////////////////////////////////////////////////////////
// This function copies a preset label (15 chars)
// Could be loaded from SD-Card later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_CopyPreset(u8 num, char *dst)
{

  if( num < SEQ_LABEL_NumPresets() ) {
    memcpy(dst, (char *)&preset_labels[num], 15);
  } else {
    memcpy(dst, "...............", 15);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns number of category presets
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_NumPresetsCategory(void)
{
  return sizeof(preset_categories) / 6;
}

/////////////////////////////////////////////////////////////////////////////
// This function copies a preset category (5 chars) into the given array
// Could be loaded from SD-Card later
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LABEL_CopyPresetCategory(u8 num, char *dst)
{
  if( num < SEQ_LABEL_NumPresetsCategory() ) {
    memcpy(dst, (char *)&preset_categories[num], 5);
  } else {
    memcpy(dst, ".....", 5);
  }

  return 0; // no error
}

