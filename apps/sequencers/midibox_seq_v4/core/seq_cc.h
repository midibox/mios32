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
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// usage depends on event mode (e.g. drum tracks: MIDI channel for up to 16 trigger layers)
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

// usage depends on event mode (e.g. drum tracks: MIDI note for up to 16 trigger layers)
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

// usage depends on event mode (e.g. drum tracks: CC1 number)
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

// usage depends on event mode (e.g. drum tracks: CC2 number or static velocity)
#define SEQ_CC_LAY_CONST_D1	0x30
#define SEQ_CC_LAY_CONST_D2	0x31
#define SEQ_CC_LAY_CONST_D3	0x32
#define SEQ_CC_LAY_CONST_D4	0x33
#define SEQ_CC_LAY_CONST_D5	0x34
#define SEQ_CC_LAY_CONST_D6	0x35
#define SEQ_CC_LAY_CONST_D7	0x36
#define SEQ_CC_LAY_CONST_D8	0x37
#define SEQ_CC_LAY_CONST_D9	0x38
#define SEQ_CC_LAY_CONST_D10	0x39
#define SEQ_CC_LAY_CONST_D11	0x3a
#define SEQ_CC_LAY_CONST_D12	0x3b
#define SEQ_CC_LAY_CONST_D13	0x3c
#define SEQ_CC_LAY_CONST_D14	0x3d
#define SEQ_CC_LAY_CONST_D15	0x3e
#define SEQ_CC_LAY_CONST_D16	0x3f

#define SEQ_CC_MODE		0x40
#define SEQ_CC_MODE_FLAGS	0x41
#define SEQ_CC_MIDI_EVENT_MODE	0x42
//#define SEQ_CC_MIDI_EVNT_CONST1	0x43 // obsolete
//#define SEQ_CC_MIDI_EVNT_CONST2	0x44 // obsolete
//#define SEQ_CC_MIDI_EVNT_CONST3	0x45 // obsolete
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
#define SEQ_CC_MORPH_SPARE	0x55
#define SEQ_CC_HUMANIZE_VALUE	0x56
#define SEQ_CC_HUMANIZE_MODE	0x57

// free: 0x58..0x5f

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

// free: 0x77..0x7f


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
  u8 lay_const[64];           // 4*16 constant parameters

  seq_core_shared_t shared;   // shared mode parameters (each track holds another value)
  seq_core_trkmode_flags_t mode; // track mode and flags
  seq_event_mode_t event_mode;  // event mode
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
