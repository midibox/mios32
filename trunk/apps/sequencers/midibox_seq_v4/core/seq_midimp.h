// $Id$
/*
 * Header file for MIDI File Importer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDIMP_H
#define _SEQ_MIDIMP_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_MIDIMP_MODE_AllNotes,
  SEQ_MIDIMP_MODE_AllDrums
} seq_midimp_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDIMP_Init(u32 mode);

extern seq_midimp_mode_t SEQ_MIDIMP_ModeGet(void);
extern s32 SEQ_MIDIMP_ModeSet(seq_midimp_mode_t mode);
extern s32 SEQ_MIDIMP_ResolutionGet(void);
extern s32 SEQ_MIDIMP_ResolutionSet(u8 resolution);
extern s32 SEQ_MIDIMP_NumLayersGet(void);
extern s32 SEQ_MIDIMP_NumLayersSet(u8 num_layers);
extern s32 SEQ_MIDIMP_MaxBarsGet(void);

extern s32 SEQ_MIDIMP_ReadFile(char *path);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_MIDIMP_H */
