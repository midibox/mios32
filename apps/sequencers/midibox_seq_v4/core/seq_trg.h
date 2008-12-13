// $Id$
/*
 * Header file for trigger layer routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_TRG_H
#define _SEQ_TRG_H

#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_TRG_NUM_LAYERS  3


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:16;
  };
  struct {
    unsigned gate:2;
    unsigned skip:2;
    unsigned accent:2;
    unsigned glide:2;
    unsigned roll:2;
    unsigned random_gate:2;
    unsigned random_value:2;
    unsigned spare:2;
  };
} seq_trg_assignments_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_TRG_Init(u32 mode);

extern s32 SEQ_TRG_Get(u8 track, u8 step, u8 trg_layer);
extern s32 SEQ_TRG_GateGet(u8 track, u8 step);

extern s32 SEQ_TRG_Set(u8 track, u8 step, u8 trg_layer, u8 value);
extern s32 SEQ_TRG_GateSet(u8 track, u8 step, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 trg_layer_value[SEQ_CORE_NUM_TRACKS][SEQ_TRG_NUM_LAYERS][SEQ_CORE_NUM_STEPS/8];

#endif /* _SEQ_TRG_H */
