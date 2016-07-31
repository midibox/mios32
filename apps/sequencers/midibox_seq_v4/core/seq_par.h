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

typedef enum {
  SEQ_PAR_Type_None=0,
  SEQ_PAR_Type_Note=1,
  SEQ_PAR_Type_Chord1=2,
  SEQ_PAR_Type_Velocity=3,
  SEQ_PAR_Type_Length=4,
  SEQ_PAR_Type_CC=5,
  SEQ_PAR_Type_PitchBend=6,
  SEQ_PAR_Type_Probability=7,
  SEQ_PAR_Type_Delay=8,
  SEQ_PAR_Type_Roll=9,
  SEQ_PAR_Type_Roll2=10,
  SEQ_PAR_Type_ProgramChange=11,
  SEQ_PAR_Type_Nth1=12,
  SEQ_PAR_Type_Nth2=13,
  SEQ_PAR_Type_Chord2=14,
  SEQ_PAR_Type_Aftertouch=15,
} seq_par_layer_type_t;

#define SEQ_PAR_NUM_TYPES 16


// NOTE: numbers have to be aligned with the strings in SEQ_LCD_PrintNthMode!
#define SEQ_PAR_TYPE_NTH_OFF    0
#define SEQ_PAR_TYPE_NTH_MUTE   1
#define SEQ_PAR_TYPE_NTH_PLAY   2
#define SEQ_PAR_TYPE_NTH_ACCENT 3
#define SEQ_PAR_TYPE_NTH_ROLL   4
#define SEQ_PAR_TYPE_NTH_FX     5
#define SEQ_PAR_TYPE_NTH_NO_FX  6
#define SEQ_PAR_TYPE_NTH_RESERVED 7


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

extern s32 SEQ_PAR_NoteGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_VelocityGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_LengthGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_ChordGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_ProbabilityGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_StepDelayGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_RollModeGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_Roll2ModeGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_Nth1ValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);
extern s32 SEQ_PAR_Nth2ValueGet(u8 track, u8 step, u8 par_instrument, u16 layer_muted);

extern char *SEQ_PAR_TypeStr(seq_par_layer_type_t par_type);
extern s32 SEQ_PAR_AssignedTypeStr(u8 track, u8 par_layer, char *str_buffer);
extern u8 SEQ_PAR_MappedTypeGet(u8 par_type);
extern u8 SEQ_PAR_UnMappedTypeGet(u8 mapped_par_type);

extern u8 SEQ_PAR_InitValueGet(seq_par_layer_type_t par_type, u8 par_layer);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_B, remaining functions should
// use SEQ_PAR_Get/Set
extern u8 seq_par_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_PAR_MAX_BYTES];

#endif /* _SEQ_PAR_H */
