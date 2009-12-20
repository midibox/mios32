// $Id$
/*
 * Header file for MBSID Knob Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SID_KNOB_H
#define _SID_KNOB_H

#include <sid.h>


/////////////////////////////////////////////////////////////////////////////
// Global Defines
/////////////////////////////////////////////////////////////////////////////

#define SID_KNOB_NUM 8

typedef enum {
  SID_KNOB_1 = 0,
  SID_KNOB_2,
  SID_KNOB_3,
  SID_KNOB_4,
  SID_KNOB_5,
  SID_KNOB_VELOCITY,
  SID_KNOB_PITCHBENDER,
  SID_KNOB_AFTERTOUCH
} sid_knob_num_t;


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct sid_knob_t {
  u8 assign1;
  u8 assign2;
  u8 value;
  u8 min;
  u8 max;
} sid_knob_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SID_KNOB_Init(u32 mode);

extern s32 SID_KNOB_SetValue(u8 sid, sid_knob_num_t knob_num, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _SID_KNOB_H */
