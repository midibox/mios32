// $Id: seq_midply.h 761 2009-10-26 22:32:03Z tk $
/*
 * Header file for MIDI File Player
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

typedef enum {
  SEQ_MIDPLY_MODE_Exclusive,
  SEQ_MIDPLY_MODE_Parallel
} seq_midply_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_MIDPLY_Init(u32 mode);

extern seq_midply_mode_t SEQ_MIDPLY_ModeGet(void);
extern s32 SEQ_MIDPLY_ModeSet(seq_midply_mode_t mode);

extern s32 SEQ_MIDPLY_RunModeGet(void);
s32 SEQ_MIDPLY_RunModeSet(u8 on, u8 synched_start);

extern s32 SEQ_MIDPLY_LoopModeGet(void);
extern s32 SEQ_MIDPLY_LoopModeSet(u8 loop_mode);

extern mios32_midi_port_t SEQ_MIDPLY_PortGet(void);
extern s32 SEQ_MIDPLY_PortSet(mios32_midi_port_t port);

extern u32 SEQ_MIDPLY_SynchTickGet(void);

extern s32 SEQ_MIDPLY_ReadFile(char *path);
extern s32 SEQ_MIDPLY_DisableFile(void);

extern char *SEQ_MIDPLY_PathGet(void);

extern s32 SEQ_MIDPLY_Reset(void);
extern s32 SEQ_MIDPLY_PlayOffEvents(void);
extern s32 SEQ_MIDPLY_Reset(void);
extern s32 SEQ_MIDPLY_SongPos(u16 new_song_pos, u8 from_midi);
extern s32 SEQ_MIDPLY_Tick(u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SEQ_MIDPLY_H */
