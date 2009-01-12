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
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define SEQ_CC_MODE		0x20
#define SEQ_CC_MODE_FLAGS	0x21
#define SEQ_CC_MIDI_EVNT_MODE	0x22
#define SEQ_CC_MIDI_EVNT_CONST1	0x23
#define SEQ_CC_MIDI_EVNT_CONST2	0x24
#define SEQ_CC_MIDI_EVNT_CONST3	0x25
#define SEQ_CC_MIDI_CHANNEL	0x26
#define SEQ_CC_MIDI_PORT	0x27

#define SEQ_CC_DIRECTION	0x28
#define SEQ_CC_STEPS_REPLAY	0x29
#define SEQ_CC_STEPS_FORWARD	0x2a
#define SEQ_CC_STEPS_JMPBCK	0x2b
#define SEQ_CC_CLK_DIVIDER	0x2c
#define SEQ_CC_LENGTH		0x2d
#define SEQ_CC_LOOP		0x2e
#define SEQ_CC_CLKDIV_FLAGS	0x2f

#define SEQ_CC_TRANSPOSE_SEMI	0x30
#define SEQ_CC_TRANSPOSE_OCT	0x31
#define SEQ_CC_GROOVE_VALUE	0x32
#define SEQ_CC_GROOVE_STYLE	0x33
#define SEQ_CC_MORPH_MODE	0x34
#define SEQ_CC_MORPH_SPARE	0x35
#define SEQ_CC_HUMANIZE_VALUE	0x36
#define SEQ_CC_HUMANIZE_MODE	0x37

#define SEQ_CC_ASG_GATE		0x38
#define SEQ_CC_ASG_SKIP		0x39
#define SEQ_CC_ASG_ACCENT	0x3a
#define SEQ_CC_ASG_GLIDE 	0x3b
#define SEQ_CC_ASG_ROLL  	0x3c
#define SEQ_CC_ASG_RANDOM_GATE	0x3d
#define SEQ_CC_ASG_RANDOM_VALUE	0x3e
#define SEQ_CC_ASG_NO_FX	0x3f

#define SEQ_CC_CHANGE_STEP	0x40

#define SEQ_CC_ECHO_REPEATS	0x50
#define SEQ_CC_ECHO_DELAY	0x51
#define SEQ_CC_ECHO_VELOCITY	0x52
#define SEQ_CC_ECHO_FB_VELOCITY	0x53
#define SEQ_CC_ECHO_FB_NOTE	0x54
#define SEQ_CC_ECHO_FB_GATELENGTH 0x55
#define SEQ_CC_ECHO_FB_TICKS    0x56


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  seq_core_shared_t shared; // shared mode parameters (each track holds another value)
  seq_core_trkmode_flags_t mode; // track mode and flags
  u8       evnt_mode;         // event mode
  u8       evnt_const1;       // event constant value #1
  u8       evnt_const2;       // event constant value #2
  u8       evnt_const3;       // event constant value #3
  mios32_midi_port_t midi_port; // MIDI port
  mios32_midi_chn_t midi_chn:4; // MIDI channel
  unsigned dir_mode:4;        // track direction mode (limited to 4 bits, see also seq_cc_trk_dir_t)
  unsigned steps_replay:4;    // steps replay value
  unsigned steps_forward:4;   // steps forward value
  unsigned steps_jump_back:4; // steps jump back value
  seq_core_clkdiv_t clkdiv;     // clock divider and flags
  u8       length;            // track length
  u8       loop;              // loop point
  unsigned transpose_semi:4;  // semitons transpose
  unsigned transpose_oct:4;   // octave transpose
  unsigned groove_style:4;    // groove style
  unsigned groove_value:4;    // groove intensity
  unsigned morph_mode:4;      // morph mode
  unsigned morph_spare:4;     // for future morph extensions
  seq_trg_assignments_t trg_assignments; // trigger assignments to gate/skip/acc/gilde/roll/R.G/R.V
  unsigned humanize_mode:4;   // humanize mode
  unsigned humanize_value:4;  // humanize intensity

  // new in MBSEQ V4
  unsigned echo_repeats:4;    // repeats (0..15)
  unsigned echo_delay:4;      // delay between echoed notes (different predefined lengths + Rnd1/2)
  unsigned echo_velocity:8;   // initial velocity 0%..200%, 5 step resolution (41 values)
  unsigned echo_fb_velocity:8; // feedbacked velocity (41 values)
  unsigned echo_fb_note:8;    // feedbacked note (-24..24 + random = 50 values)
  unsigned echo_fb_gatelength:8; // feedbacked gatelength 0%..200%, 5 step resolution (41 values)
  unsigned echo_fb_ticks:8;   // feedbacked ticks 0%..200%, 5 step resolution (41 values)
} seq_cc_trk_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_CC_Init(u32 mode);

extern s32 SEQ_CC_Set(u8 track, u8 cc, u8 value);
extern s32 SEQ_CC_Get(u8 track, u8 cc);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_cc_trk_t seq_cc_trk[SEQ_CORE_NUM_TRACKS];


#endif /* _SEQ_CC_H */
