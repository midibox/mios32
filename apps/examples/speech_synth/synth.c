// $Id$
/*
 * I2S (Wavetable) Synth Module
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
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
// External prototypes
/////////////////////////////////////////////////////////////////////////////

extern int MCU_WavegenFill(int fill_zeros);

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// at 48kHz sample frequency and two channels, the sample buffer has to be refilled
// at a rate of 48Khz / SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE 128  // -> 128 L/R samples, 375 Hz refill rate (2.6~ mS period)

// the wavetable itself has 1024 samples
#define WAVETABLE_SIZE 1024

#define WAVETABLE_FRQ  (48000/WAVETABLE_SIZE)


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
unsigned char* MCU_out_ptr;
unsigned char* MCU_out_end;
unsigned char MCU_outbuf[WAVETABLE_SIZE];


// quick&dirty
u8 synth_downsampling_factor = 6;
u8 synth_resolution = 15;
u16 synth_xor = 0x0000;


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

volatile unsigned short snd_ix;			// index to current sample generated
volatile unsigned short max_ix;			// index to final sample
volatile unsigned short prev_ix;
volatile unsigned char sound_stopped;	// flag for sound driver state

//pointers to the currently played wavefile
unsigned char* pPlayWave;
unsigned char* pPlayWave_End;

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
  snd_ix = 0;
  max_ix = 0;
  prev_ix = 0;
  sound_stopped = 0;

  // start I2S DMA transfers
  return MIOS32_I2S_Start((u32 *)&sample_buffer[0], SAMPLE_BUFFER_SIZE, &SYNTH_ReloadSampleBuffer);
}


//=============================================================================
// PlayWave()
// interface function for playing wave files from user task's
//=============================================================================
void SYNTH_PlayWave(unsigned char* wavedata, unsigned long sample_no){
	pPlayWave = wavedata;
	pPlayWave_End = pPlayWave + sample_no - 1;
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by MIOS32_I2S when the lower (state == 0) or 
// upper (state == 1) range of the sample buffer has been transfered, so 
// that it can be updated
/////////////////////////////////////////////////////////////////////////////
void SYNTH_ReloadSampleBuffer(u32 state)
{
  static u8 downsampling_ctr = 0;
  static s16 last_sample = 0;
  static s16 current_sample = 0;
  static s32 sample_diff = 0;

  // transfer new samples to the lower/upper sample buffer range
  u32 *buffer = (u32 *)&sample_buffer[state ? (SAMPLE_BUFFER_SIZE/2) : 0];

  if( sound_stopped ) {
    int i;
    for(i=0; i < SAMPLE_BUFFER_SIZE/2; ++i)
      *buffer++ = 0;
  } else {
    int i;
    for(i=0; (i < (SAMPLE_BUFFER_SIZE/2)) && snd_ix < max_ix; ++i) {
      if( ++downsampling_ctr >= synth_downsampling_factor ) {
	downsampling_ctr = 0;
	last_sample = current_sample;
#if 1
	current_sample = (MCU_outbuf[snd_ix] | (MCU_outbuf[snd_ix+1] << 8));
	snd_ix += 2;
#else
	current_sample = MCU_outbuf[snd_ix++] << 8;
#endif
	snd_ix &= (WAVETABLE_SIZE-1);
	sample_diff = (current_sample - last_sample) / synth_downsampling_factor;
      }
#if 0
      u32 chn1_value = (u32)current_sample;
#else
      s32 interpolated = last_sample + sample_diff * (downsampling_ctr-1);
      u32 chn1_value = (u32)interpolated;
#endif

      if( synth_resolution && synth_resolution < 16 )
	chn1_value &= ~((1 << (16-(u32)synth_resolution)) - 1);
      if( synth_xor )
	chn1_value ^= synth_xor;

      *buffer++ = ((u32)chn1_value << 16) | (chn1_value & 0xffff);
    }

    // fill remaining bytes with 0
    for(;i < SAMPLE_BUFFER_SIZE/2; ++i)
      *buffer++ = 0;

    if( snd_ix >= max_ix )
      sound_stopped = 1;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Returns 1 if sample is played, otherwise 0
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_SamplePlayed(void)
{
  return sound_stopped ? 0 : 1;
}

/////////////////////////////////////////////////////////////////////////////
// This task should be called each 8000/(WAVETABLE_SIZE/2)
/////////////////////////////////////////////////////////////////////////////
s32 SYNTH_Tick(void)
{
  unsigned short curr_ix = snd_ix & (WAVETABLE_SIZE/2);
  int filled = 0;

  if( sound_stopped ) { //if the generation was stopped
    MCU_out_ptr = MCU_outbuf;				//fill the buffer from start
    MCU_out_end = MCU_out_ptr + (WAVETABLE_SIZE-1);	//to the end
    filled = MCU_WavegenFill(1);
    if(1 == filled) return 0;				//if nothing to be played return

    //set max_ix:
    // - at the end of the buffer if nothing else to be played
    // - past the buffer if there are more samples waiting
    max_ix = (filled == 1)? (WAVETABLE_SIZE-1):(0xFFFF);
    curr_ix = snd_ix = 0;
    //validate generation
    //sound_start();
    sound_stopped = 0;
  } else if( (curr_ix != prev_ix) ) {
    // driver is playing
    // fill next half buffer only
    MCU_out_ptr = MCU_outbuf + prev_ix;
    MCU_out_end = MCU_out_ptr + (WAVETABLE_SIZE/2-1);
    filled = MCU_WavegenFill(1);
    if(1 == filled){
      //nothing to play, stop at the end of current buffer
      max_ix = prev_ix;
      return 0; // no error
    }
    max_ix = (filled == 1)? (MCU_out_end-MCU_outbuf):(0xFFFF);
  }else{
    //nothing to do
    filled = 0;
  }
  prev_ix = curr_ix;

  return 0; // no error
}
