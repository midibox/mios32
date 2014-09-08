// $Id$
/*
 * Header file for CC parameters
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_CC_H
#define _SEQ_CC_H

#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_lfo.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// usage depends on event mode (e.g. drum tracks: MIDI notes for up to 16 trigger layers)
#define SEQ_CC_LAY_CONST_A1	0x00
#define SEQ_CC_LAY_CONST_A2	0x01
#define SEQ_CC_LAY_CONST_A3	0x02
#define SEQ_CC_LAY_CONST_A4	0x03
#define SEQ_CC_LAY_CONST_A5	0x04
#define SEQ_CC_LAY_CONST_A6	0x05
#define SEQ_CC_LAY_CONST_A7	0x06
#define SEQ_CC_LAY_CONST_A8	0x07
#define SEQ_CC_LAY_CONST_A9	0x08
#define SEQ_CC_LAY_CONST_A10	0x09
#define SEQ_CC_LAY_CONST_A11	0x0a
#define SEQ_CC_LAY_CONST_A12	0x0b
#define SEQ_CC_LAY_CONST_A13	0x0c
#define SEQ_CC_LAY_CONST_A14	0x0d
#define SEQ_CC_LAY_CONST_A15	0x0e
#define SEQ_CC_LAY_CONST_A16	0x0f

// usage depends on event mode (e.g. drum tracks: Normal Velocity for up to 16 trigger layers)
#define SEQ_CC_LAY_CONST_B1	0x10
#define SEQ_CC_LAY_CONST_B2	0x11
#define SEQ_CC_LAY_CONST_B3	0x12
#define SEQ_CC_LAY_CONST_B4	0x13
#define SEQ_CC_LAY_CONST_B5	0x14
#define SEQ_CC_LAY_CONST_B6	0x15
#define SEQ_CC_LAY_CONST_B7	0x16
#define SEQ_CC_LAY_CONST_B8	0x17
#define SEQ_CC_LAY_CONST_B9	0x18
#define SEQ_CC_LAY_CONST_B10	0x19
#define SEQ_CC_LAY_CONST_B11	0x1a
#define SEQ_CC_LAY_CONST_B12	0x1b
#define SEQ_CC_LAY_CONST_B13	0x1c
#define SEQ_CC_LAY_CONST_B14	0x1d
#define SEQ_CC_LAY_CONST_B15	0x1e
#define SEQ_CC_LAY_CONST_B16	0x1f

// usage depends on event mode (e.g. drum tracks: Accented Velocity for up to 16 trigger layers)
#define SEQ_CC_LAY_CONST_C1	0x20
#define SEQ_CC_LAY_CONST_C2	0x21
#define SEQ_CC_LAY_CONST_C3	0x22
#define SEQ_CC_LAY_CONST_C4	0x23
#define SEQ_CC_LAY_CONST_C5	0x24
#define SEQ_CC_LAY_CONST_C6	0x25
#define SEQ_CC_LAY_CONST_C7	0x26
#define SEQ_CC_LAY_CONST_C8	0x27
#define SEQ_CC_LAY_CONST_C9	0x28
#define SEQ_CC_LAY_CONST_C10	0x29
#define SEQ_CC_LAY_CONST_C11	0x2a
#define SEQ_CC_LAY_CONST_C12	0x2b
#define SEQ_CC_LAY_CONST_C13	0x2c
#define SEQ_CC_LAY_CONST_C14	0x2d
#define SEQ_CC_LAY_CONST_C15	0x2e
#define SEQ_CC_LAY_CONST_C16	0x2f

#define SEQ_CC_LFO_WAVEFORM        0x30
#define SEQ_CC_LFO_AMPLITUDE       0x31
#define SEQ_CC_LFO_PHASE           0x32
#define SEQ_CC_LFO_STEPS           0x33
#define SEQ_CC_LFO_STEPS_RST       0x34
#define SEQ_CC_LFO_ENABLE_FLAGS    0x35
#define SEQ_CC_LFO_CC              0x36
#define SEQ_CC_LFO_CC_OFFSET       0x37
#define SEQ_CC_LFO_CC_PPQN         0x38

// reserved: 0x39..0x3f

#define SEQ_CC_MODE		0x40
#define SEQ_CC_MODE_FLAGS	0x41
#define SEQ_CC_MIDI_EVENT_MODE	0x42

#define SEQ_CC_LIMIT_LOWER      0x43
#define SEQ_CC_LIMIT_UPPER      0x44

#define SEQ_CC_BUSASG           0x45
#define SEQ_CC_MIDI_CHANNEL	0x46
#define SEQ_CC_MIDI_PORT	0x47

#define SEQ_CC_DIRECTION	0x48
#define SEQ_CC_STEPS_REPLAY	0x49
#define SEQ_CC_STEPS_FORWARD	0x4a
#define SEQ_CC_STEPS_JMPBCK	0x4b
#define SEQ_CC_CLK_DIVIDER	0x4c
#define SEQ_CC_LENGTH		0x4d
#define SEQ_CC_LOOP		0x4e
#define SEQ_CC_CLKDIV_FLAGS	0x4f

#define SEQ_CC_TRANSPOSE_SEMI	0x50
#define SEQ_CC_TRANSPOSE_OCT	0x51
#define SEQ_CC_GROOVE_VALUE	0x52
#define SEQ_CC_GROOVE_STYLE	0x53
#define SEQ_CC_MORPH_MODE	0x54
#define SEQ_CC_MORPH_DST	0x55
#define SEQ_CC_HUMANIZE_VALUE	0x56
#define SEQ_CC_HUMANIZE_MODE	0x57

#define SEQ_CC_PAR_ASG_DRUM_LAYER_A 0x58
#define SEQ_CC_PAR_ASG_DRUM_LAYER_B 0x59
// free: 0x5a..0x5b

#define SEQ_CC_STEPS_REPEAT      0x5c
#define SEQ_CC_STEPS_SKIP        0x5d
#define SEQ_CC_STEPS_RS_INTERVAL 0x5e
// free: 0x5f

#define SEQ_CC_ASG_GATE		0x60
#define SEQ_CC_ASG_ACCENT	0x61
#define SEQ_CC_ASG_ROLL  	0x62
#define SEQ_CC_ASG_GLIDE 	0x63
#define SEQ_CC_ASG_SKIP		0x64
#define SEQ_CC_ASG_RANDOM_GATE	0x65
#define SEQ_CC_ASG_RANDOM_VALUE	0x66
#define SEQ_CC_ASG_NO_FX	0x67
// free: 0x68..0x6f, evtl. for more trigger types

#define SEQ_CC_ECHO_REPEATS	0x70
#define SEQ_CC_ECHO_DELAY	0x71
#define SEQ_CC_ECHO_VELOCITY	0x72
#define SEQ_CC_ECHO_FB_VELOCITY	0x73
#define SEQ_CC_ECHO_FB_NOTE	0x74
#define SEQ_CC_ECHO_FB_GATELENGTH 0x75
#define SEQ_CC_ECHO_FB_TICKS    0x76

// free: 0x77

#define SEQ_CC_FX_MIDI_MODE         0x78
#define SEQ_CC_FX_MIDI_PORT         0x79
#define SEQ_CC_FX_MIDI_CHANNEL      0x7a
#define SEQ_CC_FX_MIDI_NUM_CHANNELS 0x7b

// free: 0x7c..0x7f

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 lay_const[3*16];           // 3*16 constant parameters

  seq_core_shared_t shared;   // shared mode parameters (each track holds another value)
  seq_core_trkmode_flags_t mode; // track mode and flags
  seq_event_mode_t event_mode:4;  // event mode
  seq_core_busasg_t busasg;     // T&A bus port (0..3)
  mios32_midi_port_t midi_port; // MIDI port
  mios32_midi_chn_t midi_chn:4; // MIDI channel
  mios32_midi_chn_t fx_midi_chn:4; // Fx MIDI channel
  mios32_midi_port_t fx_midi_port; // Fx MIDI port
  u8       fx_midi_num_chn;     // number of allocated MIDI channels (upward counting) - 0=1 channel, 1=2 channels, etc
  seq_core_fx_midi_mode_t fx_midi_mode; // Fx MIDI mode
  u8       dir_mode:4;        // track direction mode (limited to 4 bits, see also seq_cc_trk_dir_t)
  u8       steps_replay:4;    // steps replay value
  u8       steps_forward:4;   // steps forward value
  u8       steps_jump_back:4; // steps jump back value
  u8       steps_repeat;      // steps repeat value
  u8       steps_skip;        // steps skip value
  u8       steps_rs_interval; // interval of Repeat/Skip function
  seq_core_clkdiv_t clkdiv;     // clock divider and flags
  u8       length;            // track length
  u8       loop;              // loop point
  u8       transpose_semi:4;  // semitons transpose
  u8       transpose_oct:4;   // octave transpose
  u8       morph_mode:4;      // morph mode
  u8       humanize_mode:4;   // humanize mode
  u8       morph_dst;         // morph destination step
  u8       groove_style;      // groove style
  u8       groove_value;      // groove intensity
  u8       humanize_value;    // humanize intensity
  seq_trg_assignments_t trg_assignments; // trigger assignments to gate/skip/acc/gilde/roll/R.G/R.V

  u8       par_assignment_drum[2]; // only used in drum mode

  // new in MBSEQ V4
  u8       echo_repeats;       // repeats (0..15) - flag #6 will disable echo
  u8       echo_delay;         // delay between echoed notes (different predefined lengths + Rnd1/2)
  u8       echo_velocity;      // initial velocity 0%..200%, 5 step resolution (41 values)
  u8       echo_fb_velocity;   // feedbacked velocity (41 values)
  u8       echo_fb_note;       // feedbacked note (-24..24 + random = 50 values)
  u8       echo_fb_gatelength; // feedbacked gatelength 0%..200%, 5 step resolution (41 values)
  u8       echo_fb_ticks;      // feedbacked ticks 0%..200%, 5 step resolution (41 values)

  seq_lfo_waveform_t lfo_waveform; // off/Sine/Tri/Saw/Rec 5..95
  u8       lfo_amplitude;     // -128..0..127
  u8       lfo_phase;         // 0..255
  u8       lfo_steps;         // 0..255
  u8       lfo_steps_rst;     // 0..255
  seq_lfo_enable_flags_t lfo_enable_flags;  // see structure
  u8       lfo_cc;            // 0..127
  u8       lfo_cc_offset;     // 0..127
  u8       lfo_cc_ppqn;       // 3..384

  u8       limit_lower;       // for note value limits
  u8       limit_upper;       // for note value limits

  // temporary variables which will be updated on CC writes
  s8 link_par_layer_note;        // parameter layer which stores the first note value (-1 if not assigned)
  s8 link_par_layer_chord;       // parameter layer which stores the first chord value (-1 if not assigned)
  s8 link_par_layer_velocity;    // parameter layer which stores the first velocity value (-1 if not assigned)
  s8 link_par_layer_length;      // parameter layer which stores the first length value (-1 if not assigned)
  s8 link_par_layer_probability; // parameter layer which stores probability value (-1 if not assigned)
  s8 link_par_layer_delay;       // parameter layer which stores delay value (-1 if not assigned)
  s8 link_par_layer_roll;        // parameter layer which stores roll value (-1 if not assigned)
  s8 link_par_layer_roll2;       // parameter layer which stores roll2 value (-1 if not assigned)
} seq_cc_trk_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CC_Init(u32 mode);

extern s32 SEQ_CC_Set(u8 track, u8 cc, u8 value);
extern s32 SEQ_CC_MIDI_Set(u8 track, u8 cc, u8 value);
extern s32 SEQ_CC_MIDI_Get(u8 track, u8 cc, u8 *mapped_cc);
extern s32 SEQ_CC_Get(u8 track, u8 cc);

extern s32 SEQ_CC_LinkUpdate(u8 track);

extern s32 SEQ_CC_TrackHasVelocityParLayer(u8 track);
extern s32 SEQ_CC_TrackHasAccentTrgLayer(u8 track);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_cc_trk_t seq_cc_trk[SEQ_CORE_NUM_TRACKS];


#endif /* _SEQ_CC_H */
