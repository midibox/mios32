// $Id: seq_morph.h 1422 2012-02-12 11:22:43Z tk $
/*
 * Header file for morphing routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MORPH_H
#define _SEQ_MORPH_H

#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MORPH_Init(u32 mode);

extern u8 SEQ_MORPH_ValueGet(void);
extern s32 SEQ_MORPH_ValueSet(u8 value);

extern s32 SEQ_MORPH_EventNote(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 layer_note, s8 layer_velocity, s8 layer_length);
extern s32 SEQ_MORPH_EventCC(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 par_layer);
extern s32 SEQ_MORPH_EventPitchBend(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 par_layer);
extern s32 SEQ_MORPH_EventProgramChange(u8 track, u8 step, seq_layer_evnt_t *e, u8 instrument, s8 par_layer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SEQ_MORPH_H */
