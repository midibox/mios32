// $Id$
/*
 * Header file for core routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_RECORD_H
#define _SEQ_RECORD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned STEP_RECORD:1;
    unsigned POLY_RECORD:1;
    unsigned AUTO_START:1;
  };
} seq_record_options_t;


typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned spare:1;
  };
} seq_record_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_RECORD_Init(u32 mode);
extern s32 SEQ_RECORD_Reset(void);

extern s32 SEQ_RECORD_PrintEditScreen(void);

extern s32 SEQ_RECORD_Receive(mios32_midi_package_t midi_package, u8 track);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_record_options_t seq_record_options;
extern seq_record_state_t seq_record_state;

extern u8 seq_record_step;

#endif /* _SEQ_RECORD_H */
