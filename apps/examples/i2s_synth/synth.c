// $Id$
/*
 * I2S (Wavetable) Synth Module
 *
 * Very simple approach w/o any oversampling, anti-aliasing, BLIT or whatever
 * This is only a quick hack to test the I2S module
 *
 * See http://ccrma.stanford.edu/~stilti/papers/blit.pdf for more 
 * sophisticated approaches
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#include <math.h>

#include "synth.h"

#include <FreeRTOS.h>
#include <portmacro.h>


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// at 48kHz sample frequency and two channels, the sample buffer has to be refilled
// at a rate of 48Khz / SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE 128  // -> 128 L/R samples, 375 Hz refill rate (2.6~ mS period)

// the wavetable itself has 512 samples (93.7 Hz @ 48 kHz)
#define WAVETABLE_SIZE 512

#define WAVETABLE_FRQ  (48000.0/(float)WAVETABLE_SIZE)


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// selected waveform for L/R channel
static synth_waveform_t selected_waveform[2];

// selected frequency for L/R channel
static float osc_frequency[2];

// the wavetable itself (128 samples == 375 Hz @ 48 kHz)
static u16 wavetable_buffer[2][WAVETABLE_SIZE];

// the wavetable accumulator and position
static float wavetable_accumulator[2];
static float wavetable_pos[2];

// sample buffer
static u32 sample_buffer[SAMPLE_BUFFER_SIZE];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

void SYNTH_ReloadSampleBuffer(u32 state);


/////////////////////////////////////////////////////////////////////////////
// initializes the synth
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_Init(u32 mode)
{
  int chn;

  for(chn=0; chn<2; ++chn) {
    wavetable_pos[chn] = 0.0;
    SYNTH_WaveformSet(chn, WAVEFORM_TRIANGLE);
    SYNTH_FrequencySet(chn, 440.0);
  }

  // start I2S DMA transfers
  return MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &SYNTH_ReloadSampleBuffer);
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the selected waveform
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_WaveformSet(u8 chn, synth_waveform_t waveform)
{
  int i;

  // valid channel?
  if( chn >= 2 )
    return -1;

  // take over waveform
  selected_waveform[chn] = waveform;

  // generate wavetable
  u16 *wavetable_ptr = (u16 *)&wavetable_buffer[chn];
  switch( waveform ) {

    case WAVEFORM_SAW: {
      s16 one_step = 65536 / WAVETABLE_SIZE;
      s16 offset = (one_step / 2) - 32768;

      for(i=0; i<WAVETABLE_SIZE; ++i)
	*wavetable_ptr++ = offset + i*one_step;
    }
    break;

    case WAVEFORM_PULSE: {
      for(i=0; i<WAVETABLE_SIZE/2; ++i)
	*wavetable_ptr++ = 32767;

      for(; i<WAVETABLE_SIZE; ++i)
	*wavetable_ptr++ = -32768;

    }
    break;

    case WAVEFORM_SINE: {
      // note: could be optimized by calculating only a quarter of the wave, and mirroring/flipping it
      for(i=0; i<WAVETABLE_SIZE; ++i)
	*wavetable_ptr++ = (s16)(32767 * sin((float)i * M_TWOPI / (float)WAVETABLE_SIZE));
      break;

  break;
    }

    case WAVEFORM_TRIANGLE:
    default: {
      s16 one_step = 2*65536 / WAVETABLE_SIZE;
      s16 offset = (one_step / 2) - 32768;

      for(i=0; i<WAVETABLE_SIZE/2; ++i)
	*wavetable_ptr++ = offset + i*one_step;

      for(; i<WAVETABLE_SIZE; ++i)
	*wavetable_ptr++ = -offset - i*one_step;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the selected waveform
/////////////////////////////////////////////////////////////////////////////
synth_waveform_t SYNTH_WaveformGet(u8 chn)
{
  // valid channel?
  if( chn >= 2 )
    return -1;

  // return selected waveform
  return selected_waveform[chn];
}


/////////////////////////////////////////////////////////////////////////////
// This function sets a new frequency
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_FrequencySet(u8 chn, float frq)
{
  // valid channel?
  if( chn >= 2 )
    return -1;

  // set frequency
  osc_frequency[chn] = frq;

  // calculate new accumulator
  wavetable_accumulator[chn] = frq / (WAVETABLE_FRQ);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the current frequency
/////////////////////////////////////////////////////////////////////////////
float SYNTH_FrequencyGet(u8 chn)
{
  // valid channel?
  if( chn >= 2 )
    return -1;

  // return current frequency
  return osc_frequency[chn];
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MIOS32_I2S when the lower (state == 0) or 
// upper (state == 1) range of the sample buffer has been transfered, so 
// that it can be updated
/////////////////////////////////////////////////////////////////////////////
void SYNTH_ReloadSampleBuffer(u32 state)
{
  // transfer new samples to the lower/upper sample buffer range
  int i;
  u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];
  for(i=0; i<SAMPLE_BUFFER_SIZE/2; ++i) {
    int chn;

    for(chn=0; chn<2; ++chn) {
      if( (wavetable_pos[chn] += wavetable_accumulator[chn]) >= WAVETABLE_SIZE )
	wavetable_pos[chn] -= WAVETABLE_SIZE;
    }

    *buffer++ = (wavetable_buffer[1][(u32)wavetable_pos[1]] << 16) | 
                 wavetable_buffer[0][(u32)wavetable_pos[0]];
  }
}
