// $Id$
/*
 * Header file for song routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_SONG_H
#define _SEQ_SONG_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of songs
#define SEQ_SONG_NUM 64

// number of steps per song
#define SEQ_SONG_NUM_STEPS 128

// number of action types (must match with seq_song_action_t)
#define SEQ_SONG_NUM_ACTIONS 22


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  SEQ_SONG_ACTION_End,
  SEQ_SONG_ACTION_Loop1,
  SEQ_SONG_ACTION_Loop2,
  SEQ_SONG_ACTION_Loop3,
  SEQ_SONG_ACTION_Loop4,
  SEQ_SONG_ACTION_Loop5,
  SEQ_SONG_ACTION_Loop6,
  SEQ_SONG_ACTION_Loop7,
  SEQ_SONG_ACTION_Loop8,
  SEQ_SONG_ACTION_Loop9,
  SEQ_SONG_ACTION_Loop10,
  SEQ_SONG_ACTION_Loop11,
  SEQ_SONG_ACTION_Loop12,
  SEQ_SONG_ACTION_Loop13,
  SEQ_SONG_ACTION_Loop14,
  SEQ_SONG_ACTION_Loop15,
  SEQ_SONG_ACTION_Loop16,
  SEQ_SONG_ACTION_JmpPos,
  SEQ_SONG_ACTION_JmpSong,
  SEQ_SONG_ACTION_SelMixerMap,
  SEQ_SONG_ACTION_Tempo,
  SEQ_SONG_ACTION_Mutes,
} seq_song_action_t;

typedef union {
  unsigned long long ALL; // 8 bytes per step

  struct {
    u32 ALL_L;
    u32 ALL_H;
  };

  struct {
    u8 action; // (don't use seq_song_action_t to ensure 8 byte width)
    u8 action_value;
    u8 pattern_g1; // no arrays are used here, since banks only allocate 4 bit, and unsigned:4 cannot be addressed as array
    u8 pattern_g2;
    u8 pattern_g3;
    u8 pattern_g4;
    u8 bank_g1:4;
    u8 bank_g2:4;
    u8 bank_g3:4;
    u8 bank_g4:4;
  };
} seq_song_step_t;



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_SONG_Init(u32 mode);

extern char *SEQ_SONG_NameGet(void);

extern s32 SEQ_SONG_NumGet(void);
extern s32 SEQ_SONG_NumSet(u32 num);

extern s32 SEQ_SONG_ActiveGet(void);
extern s32 SEQ_SONG_ActiveSet(u8 active);

extern s32 SEQ_SONG_GuideTrackGet(void);
extern s32 SEQ_SONG_GuideTrackSet(u8 track);

extern seq_song_step_t SEQ_SONG_StepEntryGet(u32 step);
extern s32 SEQ_SONG_StepEntrySet(u32 step, seq_song_step_t step_entry);
extern s32 SEQ_SONG_StepEntryClear(u32 step);

extern s32 SEQ_SONG_PosGet(void);
extern s32 SEQ_SONG_PosSet(u32 pos);

extern s32 SEQ_SONG_LoopCtrGet(void);
extern s32 SEQ_SONG_LoopCtrMaxGet(void);

extern s32 SEQ_SONG_Reset(u32 bpm_start);
extern s32 SEQ_SONG_FetchPos(u8 force_immediate_change);

extern s32 SEQ_SONG_NextPos(void);
extern s32 SEQ_SONG_PrevPos(void);


extern s32 SEQ_SONG_Fwd(void);
extern s32 SEQ_SONG_Rew(void);

extern s32 SEQ_SONG_Load(u32 song);
extern s32 SEQ_SONG_Save(u32 song);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_S, remaining functions should
// use SEQ_SONG_StepGet/Set
extern seq_song_step_t seq_song_steps[SEQ_SONG_NUM_STEPS];

extern char seq_song_name[21];

extern u8 seq_song_guide_track;

#endif /* _SEQ_SONG_H */
