// $Id$
/*
 * Header file for live routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SEQ_LIVE_H
#define _SEQ_LIVE_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// for up to 16 instruments in drum tracks
#define SEQ_LIVE_REPEAT_SLOTS 16

// number of arp patterns
#define SEQ_LIVE_NUM_ARP_PATTERNS 16


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u32 ALL;
  struct {
    s8 OCT_TRANSPOSE;
    u8 VELOCITY;
    u8 FORCE_SCALE:1;
    u8 FX:1;
    u8 KEEP_CHANNEL:1;
  };
} seq_live_options_t;

typedef struct {
  u32 pattern:7;
  u32 enabled:1;
  u32 note:8;
  u32 velocity:8;
  u32 len:8;
  u32 chn:4;
} seq_live_repeat_t;

typedef struct {
  u16 gate;
  u16 accent;
} seq_live_arp_pattern_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SEQ_LIVE_Init(u32 mode);

extern s32 SEQ_LIVE_PlayEvent(u8 track, mios32_midi_package_t p);
extern s32 SEQ_LIVE_AllNotesOff(void);

extern s32 SEQ_LIVE_NewStep(u8 track, u8 prev_step, u8 new_step, u32 bpm_tick);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern seq_live_options_t seq_live_options;
extern seq_live_repeat_t seq_live_repeat[SEQ_LIVE_REPEAT_SLOTS];
extern seq_live_arp_pattern_t seq_live_arp_pattern[SEQ_LIVE_NUM_ARP_PATTERNS];

#endif /* _SEQ_LIVE_H */
