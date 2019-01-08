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
#include <string.h>

#include "synth.h"

#include <FreeRTOS.h>
#include <portmacro.h>

#include "mcu_otherdef.h"
#include "mcu_phoneme.h"
#include "mcu_synthesize.h"


/////////////////////////////////////////////////////////////////////////////
// External prototypes
/////////////////////////////////////////////////////////////////////////////

extern int MCU_WavegenFill(int fill_zeros);
extern void MCU_WcmdqStop();
extern int MCU_WcmdqUsed();

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
// Global variables and prototypes from embspeech driver
/////////////////////////////////////////////////////////////////////////////
unsigned char* MCU_out_ptr;
unsigned char* MCU_out_end;
unsigned char MCU_outbuf[WAVETABLE_SIZE];

int MCU_pitch_offset;

int MCUTranslate(char* src);

char patchName[SYNTH_NUM_GROUPS][SYNTH_PATCH_NAME_LEN+1];



/////////////////////////////////////////////////////////////////////////////
// Local Types
/////////////////////////////////////////////////////////////////////////////
typedef struct phrase_par_t {
  u8 length;
  u8 tune;
} phrase_par_t;

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

// modified via CC
// NOTE: at least 3 entries have to be played, we reserve for up to 16 entries
static MCU_PHONEME_LIST phrases[SYNTH_NUM_PHRASES][SYNTH_PHRASE_MAX_LENGTH];
static phrase_par_t phrase_par[SYNTH_NUM_PHRASES];

static MCU_PHONEME_LIST* current_phoneme;
static phrase_par_t current_phrase;
static int current_phoneme_resume;

// quick&dirty
static u8 synth_downsampling_factor = 6;
static u8 synth_resolution = 16;
static u16 synth_xor = 0x0000;


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

  // init speak
  MCU_WavegenInit();
  MCU_SynthesizeInit();

  // init phrases
  int i;
  for(i=0; i<SYNTH_NUM_PHRASES; ++i) {
    int j;
    MCU_PHONEME_LIST *p = (MCU_PHONEME_LIST *)&phrases[i][0];
    for(j=0; j<SYNTH_PHRASE_MAX_LENGTH; ++j, ++p) {
      p->ph = 88;
      p->env = 0;
      p->tone = 0;
      p->type = 2;
      p->prepause = 0;
      p->amp = 16;
      p->newword = 0;
      p->synthflags = 4;
      p->length = 133;
      p->pitch1 = 35;
      p->pitch2 = 39;
      p->sourceix = 0;
    }

    phrase_par_t *phrase = (phrase_par_t *)&phrase_par[i];
    phrase->length = 3; // at least 3 entries have to be played
    phrase->tune = 64; // mid value
  }

  char *tmp = (char *)patchName;
  for(i=0; i<SYNTH_NUM_GROUPS * SYNTH_PATCH_NAME_LEN; ++i, ++tmp)
      *tmp = ' ';

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
	chn1_value &= ~((1 << (15-(u32)synth_resolution)) - 1);
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
    if( filled != 1 ) {
      //set max_ix:
      // - at the end of the buffer if nothing else to be played
      // - past the buffer if there are more samples waiting
      max_ix = (filled == 1)? (WAVETABLE_SIZE-1):(0xFFFF);
      curr_ix = snd_ix = 0;
      //validate generation
      //sound_start();
      sound_stopped = 0;
    }
  } else if( (curr_ix != prev_ix) ) {
    // driver is playing
    // fill next half buffer only
    MCU_out_ptr = MCU_outbuf + prev_ix;
    MCU_out_end = MCU_out_ptr + (WAVETABLE_SIZE/2-1);
    filled = MCU_WavegenFill(1);
    if(1 == filled){
      //nothing to play, stop at the end of current buffer
      max_ix = prev_ix;
    } else {
      max_ix = (filled == 1)? (MCU_out_end-MCU_outbuf):(0xFFFF);
    }
  } else {
    //nothing to do
    filled = 0;
  }
  prev_ix = curr_ix;


  if( current_phoneme ) {
    int length = current_phrase.length;
    MCU_pitch_offset = 15000 * ((int)current_phrase.tune - 64);
    current_phoneme_resume = MCU_Generate(current_phoneme, &length, current_phoneme_resume);
    if( !current_phoneme_resume )
      current_phoneme = NULL;
  }

  return 0; // no error
}


s32 SYNTH_PhrasePlay(u8 num, u8 velocity)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  // should be atomic
  MIOS32_IRQ_Disable();
  current_phoneme = (MCU_PHONEME_LIST*)&phrases[num];
  current_phrase = phrase_par[num];
  current_phoneme_resume = 0;
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 SYNTH_PhraseStop(u8 num)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  // should be atomic
  MIOS32_IRQ_Disable();
  MCU_WcmdqStop();
  MIOS32_IRQ_Enable();

  return 0; // no error
}

s32 SYNTH_PhraseIsPlayed(u8 num)
{
  if( num >= SYNTH_NUM_PHRASES )
    return 0; // not played...

  return sound_stopped ? 0 : 1;
}

s32 SYNTH_GlobalParGet(u8 par)
{
  switch( par ) {
  case SYNTH_GLOBAL_PAR_DOWNSAMPLING_FACTOR: return synth_downsampling_factor;
  case SYNTH_GLOBAL_PAR_RESOLUTION:          return synth_resolution;
  case SYNTH_GLOBAL_PAR_XOR:                 return synth_xor >> 9;
  default: return -1;
  }

  return 0; // no error
}

s32 SYNTH_GlobalParSet(u8 par, u8 value)
{
  switch( par ) {
  case SYNTH_GLOBAL_PAR_DOWNSAMPLING_FACTOR: synth_downsampling_factor = value; break;
  case SYNTH_GLOBAL_PAR_RESOLUTION:          synth_resolution = value; break;
  case SYNTH_GLOBAL_PAR_XOR:                 synth_xor = (u16)value << 9; break;
  default: return -1;
  }

  return 0; // no error
}


s32 SYNTH_PhonemeParGet(u8 num, u8 ix, u8 par)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  if( ix >= SYNTH_PHRASE_MAX_LENGTH )
    return -2;

  MCU_PHONEME_LIST *p = (MCU_PHONEME_LIST *)&phrases[num][ix];

  switch( par ) {
  case SYNTH_PHONEME_PAR_PH:        return p->ph;
  case SYNTH_PHONEME_PAR_ENV:       return p->env;
  case SYNTH_PHONEME_PAR_TONE:      return p->tone;
  case SYNTH_PHONEME_PAR_TYPE:      return p->type;
  case SYNTH_PHONEME_PAR_PREPAUSE:  return p->prepause;
  case SYNTH_PHONEME_PAR_AMP:       return p->amp;
  case SYNTH_PHONEME_PAR_NEWWORD:   return p->newword;
  case SYNTH_PHONEME_PAR_FLAGS:     return p->synthflags;
  case SYNTH_PHONEME_PAR_LENGTH:    return p->length / 64;
  case SYNTH_PHONEME_PAR_PITCH1:    return p->pitch1;
  case SYNTH_PHONEME_PAR_PITCH2:    return p->pitch2;
  case SYNTH_PHONEME_PAR_SOURCE_IX: return p->sourceix;
  }

  return -2; // invalid parameter
}

s32 SYNTH_PhonemeParSet(u8 num, u8 ix, u8 par, u8 value)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  if( ix >= SYNTH_PHRASE_MAX_LENGTH )
    return -2;

  MCU_PHONEME_LIST *p = (MCU_PHONEME_LIST *)&phrases[num][ix];

  switch( par ) {
  case SYNTH_PHONEME_PAR_PH:        p->ph = value; break;
  case SYNTH_PHONEME_PAR_ENV:       p->env = value; break;
  case SYNTH_PHONEME_PAR_TONE:      p->tone = value; break;
  case SYNTH_PHONEME_PAR_TYPE:      p->type = value; break;
  case SYNTH_PHONEME_PAR_PREPAUSE:  p->prepause = value; break;
  case SYNTH_PHONEME_PAR_AMP:       p->amp = value; break;
  case SYNTH_PHONEME_PAR_NEWWORD:   p->newword = value; break;
  case SYNTH_PHONEME_PAR_FLAGS:     p->synthflags = value; break;
  case SYNTH_PHONEME_PAR_LENGTH:    p->length = (u32)value * 64; break;
  case SYNTH_PHONEME_PAR_PITCH1:    p->pitch1 = value; break;
  case SYNTH_PHONEME_PAR_PITCH2:    p->pitch2 = value; break;
  case SYNTH_PHONEME_PAR_SOURCE_IX: p->sourceix = value; break;
  default: return 0; // init value for future extensions
  }

  return 0; // no error
}

s32 SYNTH_PhraseParGet(u8 num, u8 par)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  phrase_par_t *phrase = (phrase_par_t *)&phrase_par[num];

  switch( par ) {
  case SYNTH_PHRASE_PAR_LENGTH: return phrase->length;
  case SYNTH_PHRASE_PAR_TUNE:   return phrase->tune;
  }

  return -2; // invalid parameter
}

s32 SYNTH_PhraseParSet(u8 num, u8 par, u8 value)
{
  if( num >= SYNTH_NUM_PHRASES )
    return -1;

  phrase_par_t *phrase = (phrase_par_t *)&phrase_par[num];

  switch( par ) {
  case SYNTH_PHRASE_PAR_LENGTH:       phrase->length = (value < SYNTH_PHRASE_MAX_LENGTH) ? value : (SYNTH_PHRASE_MAX_LENGTH-1); break;
  case SYNTH_PHRASE_PAR_TUNE:         phrase->tune = value;
  default: return -2;
  }

  return 0; // no error
}

char *SYNTH_PatchNameGet(u8 group)
{
  return patchName[group];
}

s32 SYNTH_PatchNameSet(u8 group, char *newName)
{
  memcpy(patchName[group], newName, SYNTH_PATCH_NAME_LEN);
  return 0; // no error
}
