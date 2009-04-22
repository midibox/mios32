// $Id$
/*
 * Header file for parameter layer routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_PAR_H
#define _SEQ_PAR_H

#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// reserved memory for each track:
#define SEQ_PAR_MAX_BYTES   1024
// example configurations:
//   - 256 steps, 4 parameter layers: 4*256 = 1024
//   - 64 steps, 16 parameter layers: 16*64 = 1024
// don't change this value - it directly affects the constraints of the bank file format!


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

#define SEQ_PAR_NUM_TYPES 12
typedef enum {
  SEQ_PAR_Type_None=0,
  SEQ_PAR_Type_Note=1,
  SEQ_PAR_Type_Chord=2,
  SEQ_PAR_Type_Velocity=3,
  SEQ_PAR_Type_Length=4,
  SEQ_PAR_Type_CC=5,
  SEQ_PAR_Type_PitchBend=6,
  SEQ_PAR_Type_Probability=7,
  SEQ_PAR_Type_Delay=8,
  SEQ_PAR_Type_Roll=9,
  SEQ_PAR_Type_BaseNote=10,
  SEQ_PAR_Type_Loopback=11
} seq_par_layer_type_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_PAR_Init(u32 mode);

extern s32 SEQ_PAR_TrackInit(u8 track, u16 steps, u8 par_layers, u8 instruments);

extern s32 SEQ_PAR_NumInstrumentsGet(u8 track);
extern s32 SEQ_PAR_NumLayersGet(u8 track);
extern s32 SEQ_PAR_NumStepsGet(u8 track);

extern seq_par_layer_type_t SEQ_PAR_AssignmentGet(u8 track, u8 par_layer);

extern s32 SEQ_PAR_Set(u8 track, u16 step, u8 par_layer, u8 par_instrument, u8 value);
extern s32 SEQ_PAR_Get(u8 track, u16 step, u8 par_layer, u8 par_instrument);

extern s32 SEQ_PAR_NoteGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_VelocityGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_LengthGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_ProbabilityGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_StepDelayGet(u8 track, u8 step, u8 par_instrument);
extern s32 SEQ_PAR_RollModeGet(u8 track, u8 step, u8 par_instrument);

extern char *SEQ_PAR_TypeStr(seq_par_layer_type_t par_type);
extern char *SEQ_PAR_AssignedTypeStr(u8 track, u8 par_layer);

extern u8 SEQ_PAR_InitValueGet(seq_par_layer_type_t par_type, u8 par_layer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_B, remaining functions should
// use SEQ_PAR_Get/Set
extern u8 seq_par_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_PAR_MAX_BYTES];

#endif /* _SEQ_PAR_H */
