// $Id$
/*
 * Header file of Synth Module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SYNTH_H
#define _SYNTH_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of provided waveforms
#define SYNTH_NUM_WAVEFORMS  4


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


// available waveforms
typedef enum {
  WAVEFORM_TRIANGLE,
  WAVEFORM_SAW,
  WAVEFORM_PULSE,
  WAVEFORM_SINE
} synth_waveform_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 SYNTH_Init(u32 mode);

extern s32 SYNTH_SamplePlayed(void);
extern s32 SYNTH_Tick(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern u8 synth_downsampling_factor;
extern u8 synth_resolution;
extern u16 synth_xor;

#endif /* _SYNTH_H */
