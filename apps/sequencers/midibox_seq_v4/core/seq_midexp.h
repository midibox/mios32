// $Id$
/*
 * Header file for MIDI File Exporter
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDEXP_H
#define _SEQ_MIDEXP_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_MIDEXP_MODE_Track,
  SEQ_MIDEXP_MODE_Group,
  SEQ_MIDEXP_MODE_AllGroups,
  SEQ_MIDEXP_MODE_Song
} seq_midexp_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDEXP_Init(u32 mode);

extern seq_midexp_mode_t SEQ_MIDEXP_ModeGet(void);
extern s32 SEQ_MIDEXP_ModeSet(seq_midexp_mode_t mode);
extern s32 SEQ_MIDEXP_ExportMeasuresGet(void);
extern s32 SEQ_MIDEXP_ExportMeasuresSet(u16 measures);
extern s32 SEQ_MIDEXP_ExportStepsPerMeasureGet(void);
extern s32 SEQ_MIDEXP_ExportStepsPerMeasureSet(u8 steps_per_measure);

extern s32 SEQ_MIDEXP_ExportTrackGet(void);

extern s32 SEQ_MIDEXP_GenerateFile(char *path);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_MIDEXP_H */
