// $Id$
/*
 * Header file for MIDI player
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_MIDPLY_H
#define _SEQ_MIDPLY_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:32;
  };
  struct {
    unsigned RUN:1;
    unsigned PAUSE:1;
  };
} seq_midply_state_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDPLY_Init(u32 mode);

extern s32 SEQ_MIDPLY_Reset(void);

extern s32 SEQ_MIDPLY_Start(u8 no_echo);
extern s32 SEQ_MIDPLY_Stop(u8 no_echo);
extern s32 SEQ_MIDPLY_Cont(u8 no_echo);
extern s32 SEQ_MIDPLY_Pause(u8 no_echo);
extern s32 SEQ_MIDPLY_SongPos(u16 new_song_pos, u8 no_echo);

extern s32 SEQ_MIDPLY_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_midply_state_t seq_midply_state;

#endif /* _SEQ_MIDPLY_H */
