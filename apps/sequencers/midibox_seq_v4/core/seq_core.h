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

#define SEQ_CORE_NUM_GROUPS            4
#define SEQ_CORE_NUM_TRACKS_PER_GROUP  4
#define SEQ_CORE_NUM_TRACKS            (SEQ_CORE_NUM_TRACKS_PER_GROUP*SEQ_CORE_NUM_GROUPS)


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned SYNCHED_PATTERN_CHANGE:1;
    unsigned FOLLOW_SONG:1;
    unsigned PASTE_CLR_ALL:1;
  };
} seq_core_options_t;


typedef union {
  struct {
    unsigned ALL:16;
  };
  struct {
    u8       ref_step:8;

    unsigned FIRST_CLK:1;
  };
} seq_core_state_t;


typedef union {
  struct {
    unsigned ALL:16;
  };
  struct {
    unsigned DISABLED:1;    // set if no pattern is selected to avoid editing of trigger/layer values
    unsigned MUTED:1;       // track is muted
    unsigned POS_RESET:1;   // set by MIDI handler if position of ARP/Transpose track should be reset
    unsigned BACKWARD:1;    // if set, the track will be played in backward direction
    unsigned FIRST_CLK:1;   // don't increment on the first clock event
    unsigned REC_EVENT_ACTIVE:1; // set so long a note is held (for note length measuring)
    unsigned REC_MUTE_NEXT_STEP:1; // if step has already been played
    unsigned SYNC_MEASURE:1; // temporary request for synch to measure (used during pattern switching)
    unsigned SUSTAINED:1;    // sustained note
    unsigned STRETCHED_GL:1; // stretched gatelength
  };
} seq_core_trk_state_t;


typedef struct seq_core_trk_t {
  seq_core_trk_state_t state;            // various status flags (see structure definition above)
  char                 name[81];         // the track name (80 chars + zero terminator)
  u8                   step;             // current track position
  u16                  step_length;      // length of the current step
  u32                  timestamp_next_step; // timestamp at which the next step will be played
  u32                  timestamp_next_step_ref; // timestamp of next step w/o groove delay
  u16                  bpm_tick_delay;   // delay of current step
  u8                   step_replay_ctr;  // step replay counter
  u8                   step_saved;       // for replay mechanism
  u8                   step_fwd_ctr;     // step forward counter
  u8                   arp_pos;          // arpeggiator position
  u8                   vu_meter;         // for visualisation in mute menu
  u8                   rec_last_note;    // for recording function
  u32                  rec_timestamp;    // for recording function
  u8                   rec_poly_ctr;     // for recording function
} seq_core_trk_t;


typedef enum {
  SEQ_CORE_TRKMODE_Off,
  SEQ_CORE_TRKMODE_Normal,
  SEQ_CORE_TRKMODE_Transpose,
  SEQ_CORE_TRKMODE_Arpeggiator
} seq_core_trk_playmode_t;


typedef enum {
  SEQ_CORE_TRKDIR_Forward,
  SEQ_CORE_TRKDIR_Backward,
  SEQ_CORE_TRKDIR_PingPong,
  SEQ_CORE_TRKDIR_Pendulum,
  SEQ_CORE_TRKDIR_Random_Dir,
  SEQ_CORE_TRKDIR_Random_Step,
  SEQ_CORE_TRKDIR_Random_D_S
} seq_core_trk_dir_t;


// shared mode parameters (each track holds another value)
typedef union {
  struct {
    u8 chain; // stored in track #1
  };
  struct {
    u8 morph_pattern; // stored in track #2
  };
  struct {
    u8 scale; // stored in track #3
  };
  struct {
    u8 scale_root; // stored in track #4
  };
} seq_core_shared_t;


typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    unsigned playmode:2;     // see seq_core_trk_playmode_t (limited to 2 bits here)
    unsigned flags:6;        // combines all flags (for CC access)
  };
  struct {
    unsigned playmode:2;     // see seq_core_trk_playmode_t (limited to 2 bits here)
    unsigned UNSORTED:1;     // sort mode for arpeggiator
    unsigned HOLD:1;         // hold mode for transposer/arpeggiator
    unsigned RESTART:1;      // track restart on key press
    unsigned FORCE_SCALE:1;  // note values are forced to scale
    unsigned SUSTAIN:1;      // events are sustained
  };
} seq_core_trkmode_flags_t;


typedef union {
  struct {
    unsigned ALL:8;
  };
  struct {
    u8 value;                // clock divider value
    unsigned flags:6;        // combines all flags (for CC access)
  };
  struct {
    u8       value;          // clock divider value
    unsigned SYNCH_TO_MEASURE:1; // synch to globally selectable measure
    unsigned TRIPLETS:1;     // play triplets
  };
} seq_core_clkdiv_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CORE_Init(u32 mode);

extern s32 SEQ_CORE_Reset(void);

extern s32 SEQ_CORE_Handler(void);

extern s32 SEQ_CORE_FTS_GetScaleAndRoot(u8 *scale, u8 *root_selection, u8 *root);

extern s32 SEQ_CORE_AddForwardDelay(u16 delay_ms);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_core_options_t seq_core_options;
extern u8 seq_core_steps_per_measure;

extern u8 seq_core_global_scale;
extern u8 seq_core_global_scale_ctrl;
extern u8 seq_core_global_scale_root_selection;
extern u8 seq_core_keyb_scale_root;

extern u8 seq_core_bpm_div_int;
extern u8 seq_core_bpm_div_ext;

extern seq_core_state_t seq_core_state;
extern seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];

#endif /* _SEQ_CORE_H */
