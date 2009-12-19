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

#define SID_KNOB_IX_1  0
#define SID_KNOB_IX_2  1
#define SID_KNOB_IX_3  2
#define SID_KNOB_IX_4  3
#define SID_KNOB_IX_5  4
#define SID_KNOB_IX_V  5
#define SID_KNOB_IX_P  6
#define SID_KNOB_IX_A  7


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

extern s32 SID_KNOB_KnobInit(sid_knob_t *knob);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern sid_knob_t sid_knob[SID_NUM][SID_KNOB_NUM];

#endif /* _SID_KNOB_H */
