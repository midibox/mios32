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

#define SEQ_CORE_NUM_BPM_PRESETS       16


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u32 ALL;
  struct {
    u32 INIT_CC:8;
    u32 SYNCHED_PATTERN_CHANGE:1;
    u32 PASTE_CLR_ALL:1;
    u32 RATOPC:1;
    u32 SYNCHED_MUTE:1;
    u32 SYNCHED_UNMUTE:1;
    u32 PATTERN_MIXER_MAP_COUPLING:1;
    u32 MIXER_LIVE_SEND:1;
    u32 INIT_WITH_TRIGGERS:1;
    u32 LIVE_LAYER_MUTE_STEPS:3; // 0=off, 1=permanent, 2..4 steps
  };
} seq_core_options_t;

typedef union {
  u32 ALL;
  struct {
    u16 ref_step; // u16 instead of u8 to cover overrun on 256 steps per measure
    u16 ref_step_song; // reference step can be different in song mode if a guide track is used
    u16 reset_trkpos_req; // resets the track with the next step

    u16 FIRST_CLK:1;
    u16 FORCE_REF_STEP_RESET:1;
    u16 METRONOME:1;
    u16 MANUAL_TRIGGER_STOP_REQ:1;
    u16 MANUAL_TRIGGER_STEP_REQ:1;
    u16 EXT_RESTART_REQ:1;
    u16 LOOP:1;
    u16 FOLLOW:1;
  };
} seq_core_state_t;


typedef union {
  u16 ALL;
  struct {
    u16 DISABLED:1;    // set if no pattern is selected to avoid editing of trigger/layer values
    u16 POS_RESET:1;   // set by MIDI handler if position of ARP/Transpose track should be reset
    u16 BACKWARD:1;    // if set, the track will be played in backward direction
    u16 FIRST_CLK:1;   // don't increment on the first clock event
    u16 REC_DONT_OVERWRITE_NEXT_STEP:1; // if a recorded step has been shifted forward
    u16 SYNC_MEASURE:1; // temporary request for synch to measure (used during pattern switching)
    u16 SUSTAINED:1;    // sustained note
    u16 ROBOSUSTAINED:1;  // events are temporarily sustained by the robotizer
    u16 STRETCHED_GL:1; // stretched gatelength
    u16 MANUAL_STEP_REQ:1; // manual_step should be copied to step
    u16 CANCEL_SUSTAIN_REQ:1; // cancel ongoing sustain
  };
} seq_core_trk_state_t;


typedef struct seq_core_trk_t {
  seq_core_trk_state_t state;            // various status flags (see structure definition above)
  char                 name[81];         // the track name (80 chars + zero terminator)
  u8                   step;             // current track position
  u8                   bar;              // bar counter
  u16                  step_length;      // length of the current step
  u32                  timestamp_next_step; // timestamp at which the next step will be played
  u32                  timestamp_next_step_ref; // timestamp of next step w/o groove delay
  u32                  glide_notes[4];   // 128 bit to store notes in glide state
  u16                  bpm_tick_delay;   // delay of current step
  u8                   step_replay_ctr;  // step replay counter
  u8                   step_saved;       // for replay mechanism
  u8                   manual_step;      // requested step in manual trigger mode
  u8                   step_fwd_ctr;     // step forward counter
  u8                   step_interval_ctr; // step interval counter
  u8                   step_repeat_ctr;  // step repeat counter
  u8                   step_skip_ctr;    // step skip counter
  u16                  layer_muted;      // separate layer mutes
  u16                  layer_muted_from_midi; // temporary layer mutes on incoming (and matching) events
  u16                  layer_muted_from_midi_next; // will be taken over with the next step
  u8                   lfo_cc_muted_from_midi:1; // the same for the LFO CC
  u8                   lfo_cc_muted_from_midi_next:1;
  u8                   arp_pos;          // arpeggiator position
  u8                   vu_meter;         // for visualisation in mute menu
  u32                  rec_timestamp;    // for recording function
  u8                   rec_poly_ctr;     // for recording function
  u8                   play_section;     // selects the section which should be played. If -1, no section selection
  u8                   fx_midi_ctr;      // Fx MIDI channel counter
} seq_core_trk_t;


typedef enum {
  SEQ_CORE_TRKMODE_Off,
  SEQ_CORE_TRKMODE_Normal,
  SEQ_CORE_TRKMODE_Transpose,
  SEQ_CORE_TRKMODE_Arpeggiator
} seq_core_trk_playmode_t;

typedef union {
  u8 ALL;
  struct {
    u8 bus:2;
  };
} seq_core_busasg_t;

typedef enum {
  SEQ_CORE_TRKDIR_Forward,
  SEQ_CORE_TRKDIR_Backward,
  SEQ_CORE_TRKDIR_PingPong,
  SEQ_CORE_TRKDIR_Pendulum,
  SEQ_CORE_TRKDIR_Random_Dir,
  SEQ_CORE_TRKDIR_Random_Step,
  SEQ_CORE_TRKDIR_Random_D_S
} seq_core_trk_dir_t;

typedef enum {
  SEQ_CORE_SLAVECLK_MUTE_Off,
  SEQ_CORE_SLAVECLK_MUTE_Enabled,
  SEQ_CORE_SLAVECLK_MUTE_OffOnNextMeasure,
} seq_core_slaveclk_mute_t;

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
  u8 ALL;
  struct {
    u8 playmode:2;     // see seq_core_trk_playmode_t (limited to 2 bits here)
    u8 flags:6;        // combines all flags (for CC access)
  };
  struct {
    u8 playmode_dummy:2;     // see seq_core_trk_playmode_t (limited to 2 bits here)
    u8 UNSORTED:1;     // sort mode for arpeggiator
    u8 HOLD:1;         // hold mode for transposer/arpeggiator
    u8 RESTART:1;      // track restart on key press
    u8 FORCE_SCALE:1;  // note values are forced to scale
    u8 SUSTAIN:1;      // events are sustained because SUSTAIN is set as active on the track
    u8 FIRST_NOTE:1;   // transposer takes the first played note instead of the last one
  };
} seq_core_trkmode_flags_t;

typedef enum {
  SEQ_CORE_FX_MIDI_MODE_BEH_Forward,   // forward to all channels
  SEQ_CORE_FX_MIDI_MODE_BEH_Alternate, // alternate between channels
  SEQ_CORE_FX_MIDI_MODE_BEH_AlternateSynchedEcho, // Alternate only on echo taps
  SEQ_CORE_FX_MIDI_MODE_BEH_Random,    // forward to random channel
} seq_core_fx_midi_mode_beh_t;

typedef union {
  u8 ALL;
  struct {
    u8 beh:3;           // Fx behaviour (seq_core_fx_midi_mode_beh_t)
    u8 fwd_non_notes:1; // forward CCs, PitchBender, Channel Pressure, Program Change to all Fx channels
  };
} seq_core_fx_midi_mode_t;

typedef union {
  u16 ALL;
  struct {
    u8 value;                // clock divider value
    u8 flags;                // combines all flags (for CC access)
  };
  struct {
    u8 value_dummy;          // clock divider value
    u8 SYNCH_TO_MEASURE:1;   // synch to globally selectable measure
    u8 TRIPLETS:1;           // play triplets
    u8 MANUAL:1;             // clock to next step only on manual requests (or via Step CC)
  };
} seq_core_clkdiv_t;


#define SEQ_CORE_NUM_LOOP_MODES 4
typedef enum {
  SEQ_CORE_LOOP_MODE_ALL_TRACKS_VIEW,
  SEQ_CORE_LOOP_MODE_ALL_TRACKS_STATIC,
  SEQ_CORE_LOOP_MODE_SELECTED_TRACK_VIEW,
  SEQ_CORE_LOOP_MODE_SELECTED_TRACK_STATIC,
} seq_core_loop_mode_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_robotize.h"
#include <seq_midi_out.h>

extern s32 SEQ_CORE_Init(u32 mode);

extern s32 SEQ_CORE_ScheduleEvent(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t midi_package, seq_midi_out_event_type_t event_type, u32 timestamp, u32 len, u8 is_echo, seq_robotize_flags_t robotize_flags);

extern s32 SEQ_CORE_Reset(u32 bpm_start);
extern s32 SEQ_CORE_PlayOffEvents(void);
extern s32 SEQ_CORE_Tick(u32 bpm_tick, s8 export_track, u8 mute_nonloopback_tracks);

extern s32 SEQ_CORE_Handler(void);

extern s32 SEQ_CORE_FTS_GetScaleAndRoot(u8 track, u8 step, u8 instrument, seq_cc_trk_t *tcc, u8 *scale, u8 *root_selection, u8 *root);

extern const char *SEQ_CORE_Echo_GetDelayModeName(u8 delay_mode);
extern u8 SEQ_CORE_Echo_MapUserToInternal(u8 user_value);
extern u8 SEQ_CORE_Echo_MapInternalToUser(u8 internal_value);

extern s32 SEQ_CORE_Transpose(u8 track, u8 instrument, seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *p);
extern s32 SEQ_CORE_Limit(seq_core_trk_t *t, seq_cc_trk_t *tcc, seq_layer_evnt_t *e);

extern s32 SEQ_CORE_Echo(u8 track, u8 instrument, seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t p, u32 bpm_tick, u32 gatelength, seq_robotize_flags_t robotize_flags);

extern s32 SEQ_CORE_ResetTrkPosAll(void);
extern s32 SEQ_CORE_SetTrkPos(u8 track, u8 value, u8 scale_value);

extern s32 SEQ_CORE_ManualTrigger(u8 step);
extern s32 SEQ_CORE_ManualSynchToMeasure(u16 tracks);

extern s32 SEQ_CORE_NotifyIncomingMIDIEvent(u8 track, mios32_midi_package_t p);

extern s32 SEQ_CORE_AddForwardDelay(u16 delay_ms);

extern s32 SEQ_CORE_BPM_Update(float bpm, float sweep_ramp);
extern s32 SEQ_CORE_BPM_SweepHandler(void);

extern s32 SEQ_CORE_Scrub(s32 incrementer);

extern s32 SEQ_CORE_CancelSustainedNotes(u8 track);

extern u8 SEQ_CORE_TrimNote(s32 note, u8 lower, u8 upper);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_core_options_t seq_core_options;
extern u8 seq_core_steps_per_measure;
extern u8 seq_core_steps_per_pattern;

extern u16 seq_core_trk_muted;
extern u16 seq_core_trk_synched_mute;
extern u16 seq_core_trk_synched_unmute;
extern seq_core_slaveclk_mute_t seq_core_slaveclk_mute;
extern u16 seq_core_trk_soloed;

extern u8 seq_core_step_update_req;

extern u8 seq_core_global_scale;
extern u8 seq_core_global_scale_root_selection;
extern u8 seq_core_keyb_scale_root;

extern u8 seq_core_global_transpose_enabled;

extern u8 seq_core_bpm_preset_num;
extern float seq_core_bpm_preset_tempo[SEQ_CORE_NUM_BPM_PRESETS];
extern float seq_core_bpm_preset_ramp[SEQ_CORE_NUM_BPM_PRESETS];

extern mios32_midi_port_t seq_core_metronome_port;
extern u8 seq_core_metronome_chn;
extern u8 seq_core_metronome_note_m;
extern u8 seq_core_metronome_note_b;

extern seq_core_state_t seq_core_state;
extern seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];

extern seq_core_loop_mode_t seq_core_glb_loop_mode;
extern u8 seq_core_glb_loop_offset;
extern u8 seq_core_glb_loop_steps;

#endif /* _SEQ_CORE_H */
