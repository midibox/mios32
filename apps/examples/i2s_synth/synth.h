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

extern s32 SYNTH_WaveformSet(u8 chn, synth_waveform_t waveform);
extern synth_waveform_t SYNTH_WaveformGet(u8 chn);

extern s32 SYNTH_FrequencySet(u8 chn, float frq);
extern float SYNTH_FrequencyGet(u8 chn);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _SYNTH_H */
