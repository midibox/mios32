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

#ifndef _SEQ_CORE_H
#define _SEQ_CORE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_CORE_NUM_STEPS  32
#define SEQ_CORE_NUM_TRACKS 16


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:32;
  };
  struct {
    u8       ref_step:8;

    unsigned RUN:1;
    unsigned PAUSE:1;
    unsigned FIRST_CLK:1;
  };
} seq_core_state_t;

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned DISABLED:1;    // set if no pattern is selected to avoid editing of trigger/layer values
    unsigned POS_RESET:1;   // set by MIDI handler if position of ARP/Transpose track should be reset
    unsigned BACKWARD:1;    // if set, the track will be played in backward direction
    unsigned REC_EVNT_ACTIVE:1; // set so long a note/CC is held (for note length measuring)
    unsigned REC_MUTE_NEXTSTEP:1; // for length...
    unsigned SYNC_MEASURE:1; // temporary request for synch to measure (used during pattern switching)
  };
} seq_core_trk_state_t;


typedef struct seq_core_trk_t {
  seq_core_trk_state_t state;            // various status flags (see structure definition above)
  u8                   step;             // track position
  u8                   div_ctr;          // clock divider counter
  u16                  next_delay_ctr;   // number of MIDI clock ticks until next step will be played
  u8                   step_replay_ctr;  // step replay counter
  u8                   step_saved;       // for replay mechanism
  u8                   step_fwd_ctr;     // step forward counter
  u8                   arp_pos;          // arpeggiator position
} seq_core_trk_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CORE_Init(u32 mode);

extern s32 SEQ_CORE_Reset(void);

extern s32 SEQ_CORE_Start(u32 no_echo);
extern s32 SEQ_CORE_Stop(u32 no_echo);
extern s32 SEQ_CORE_Cont(u32 no_echo);
extern s32 SEQ_CORE_Pause(u32 no_echo);

extern s32 SEQ_CORE_Tick(u32 bpm_tick);

extern s32 SEQ_CORE_Handler(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_core_state_t seq_core_state;
extern seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];

#endif /* _SEQ_CORE_H */
