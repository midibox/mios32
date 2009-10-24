// $Id$
/*
 * MIDI File Exporter
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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

#include <seq_midi_out.h>
#include <mid_parser.h>
#include <dosfs.h>

#include "seq_midexp.h"
#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_song.h"
#include "seq_file.h"
#include "seq_ui.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static seq_midexp_mode_t seq_midexp_mode;

static u16 export_measures;
static u8 export_steps_per_measure;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_MIDEXP_Init(u32 mode)
{
  // init default settings
  seq_midexp_mode = SEQ_MIDEXP_MODE_Track;
  export_measures = 0; // 1 measure
  export_steps_per_measure = 15; // 16 steps

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set functions
/////////////////////////////////////////////////////////////////////////////
seq_midexp_mode_t SEQ_MIDEXP_ModeGet(void)
{
  return seq_midexp_mode;
}

s32 SEQ_MIDEXP_ModeSet(seq_midexp_mode_t mode)
{
  if( mode >= SEQ_MIDEXP_MODE_Track && mode <= SEQ_MIDEXP_MODE_Song ) {
    seq_midexp_mode = mode;
  }  else {
    return -1; // invalid mode
  }

  return 0; // no error
}

s32 SEQ_MIDEXP_ExportMeasuresGet(void)
{
  return export_measures;
}
s32 SEQ_MIDEXP_ExportMeasuresSet(u16 measures)
{
  export_measures = measures;
  return 0; // no error
}

s32 SEQ_MIDEXP_ExportStepsPerMeasureGet(void)
{
  return export_steps_per_measure;
}
s32 SEQ_MIDEXP_ExportStepsPerMeasureSet(u8 steps_per_measure)
{
  export_steps_per_measure = steps_per_measure;
  return 0; // no error
}
