// $Id$
/*
 * Header file for pattern routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_PATTERN_H
#define _SEQ_PATTERN_H


#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:16;
  };
  struct {
    unsigned pattern:7;      // full pattern number
    unsigned DISABLED:1;     // pattern can be disabled
    unsigned REQ:1;          // change pattern request flag
    unsigned SYNCHED:1;      // change should be synched to measure
    unsigned bank:3;         // pattern bank
  };
  struct {
    unsigned num:3;          // pattern number (1-8)
    unsigned group:3;        // pattern group (A-H)
    unsigned lower:1;        // selects between upper (A-H) and lower (a-h) group
  };
} seq_pattern_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_PATTERN_Init(u32 mode);

extern s32 SEQ_PATTERN_Change(u8 group, seq_pattern_t pattern);
extern s32 SEQ_PATTERN_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_pattern_t seq_pattern[SEQ_CORE_NUM_GROUPS];

#endif /* _SEQ_PATTERN_H */
