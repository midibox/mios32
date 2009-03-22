// $Id$
/*
 * Song Routines
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
#include "tasks.h"

#include "seq_song.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_mixer.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_S, remaining functions should
// use SEQ_SONG_StepEntryGet/Set
seq_song_step_t seq_song_steps[SEQ_SONG_NUM_STEPS];

char seq_song_name[21];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 song_num;
static u8 song_active; // switches between song/phrase mode

static u8 song_pos;
static u8 song_loop_ctr;
static u8 song_loop_ctr_max;

static u8 something_has_been_changed;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Init(u32 mode)
{
  // init name
  int i;
  for(i=0; i<20; ++i)
    seq_song_name[i] = ' ';
  seq_song_name[i] = 0;
 
  // select initial song number and phrase mode
  song_num = 0;
  song_active = 0;

  // initialise song steps
  int step;
  seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[0];
  for(step=0; step<SEQ_SONG_NUM_STEPS; ++step, ++s) {
    s->action = SEQ_SONG_ACTION_Stop;
    s->action_value = 0;
    s->pattern_g1 = 0x80;
    s->bank_g1 = 0;
    s->pattern_g2 = 0x80;
    s->bank_g2 = 1;
    s->pattern_g3 = 0x80;
    s->bank_g3 = 2;
    s->pattern_g4 = 0x80;
    s->bank_g4 = 3;
  }

  // don't store these empty entries
  something_has_been_changed = 0;

  // reset the song sequencer
  SEQ_SONG_Reset();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the name of the song (20 characters)
/////////////////////////////////////////////////////////////////////////////
char *SEQ_SONG_NameGet(void)
{
  return seq_song_name;
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set song number
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_NumGet(void)
{
  return song_num;
}

s32 SEQ_SONG_NumSet(u32 song)
{
  if( song >= SEQ_SONG_NUM )
    return -1; // invalid song number

  song_num = song;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set song active mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_ActiveGet(void)
{
  return song_active;
}

s32 SEQ_SONG_ActiveSet(u8 active)
{
  song_active = active ? 1 : 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set step entry of a song
/////////////////////////////////////////////////////////////////////////////
seq_song_step_t SEQ_SONG_StepEntryGet(u32 step)
{
  return seq_song_steps[(step >= SEQ_SONG_NUM_STEPS) ? 0 : step];
}

s32 SEQ_SONG_StepEntrySet(u32 step, seq_song_step_t step_entry)
{
  if( step >= SEQ_SONG_NUM_STEPS )
    return -1; // invalid step number

  seq_song_steps[step] = step_entry;

  // for auto-save operation
  something_has_been_changed = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set song position
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_PosGet(void)
{
  return song_pos;
}

s32 SEQ_SONG_PosSet(u32 pos)
{
  song_pos = pos % SEQ_SONG_NUM_STEPS;
  return SEQ_SONG_FetchPos();
}


/////////////////////////////////////////////////////////////////////////////
// Get Song Loop/Loop max counter
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_LoopCtrGet(void)
{
  return song_loop_ctr;
}

s32 SEQ_SONG_LoopCtrMaxGet(void)
{
  return song_loop_ctr_max;
}


/////////////////////////////////////////////////////////////////////////////
// reset the song sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Reset(void)
{
  // reset song position and loop counter
  song_pos = 0;
  song_loop_ctr = 0;
  song_loop_ctr_max = 0;

  // if not in phrase mode: fetch new entries
  if( song_active ) {
    if( SEQ_SONG_FetchPos() < 0 ) { // returns -1 if recursion counter reached max position
      // reset song positions and loop counters again
      song_pos = 0;
      song_loop_ctr = 0;
      song_loop_ctr_max = 0;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// fetches the pos entries of a song
// returns -1 if recursion counter reached max position
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_FetchPos(void)
{
  int recursion_ctr = 0;

  u8 again;
  do {
    again = 0;

    // stop song once recursion counter reached 64 loops
    if( ++recursion_ctr >= 64 ) {
      SEQ_BPM_Stop();
      return -1; // recursion detected
    }

    // reset song position if we reached the end (loop over 128 steps)
    if( song_pos >= SEQ_SONG_NUM_STEPS )
      song_pos = 0;

    // get step entry
    seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[song_pos];

    // branch depending on action
    switch( s->action ) {
      case SEQ_SONG_ACTION_Stop:
	SEQ_BPM_Stop();
	break;

      case SEQ_SONG_ACTION_JmpPos:
	song_pos = s->action_value % SEQ_SONG_NUM_STEPS;
	again = 1;
	break;

      case SEQ_SONG_ACTION_JmpSong: {
	u32 new_song_num = s->action_value % SEQ_SONG_NUM;

	SEQ_SONG_Save(song_num);
	SEQ_SONG_Load(new_song_num);
	song_pos = 0;
	again = 1;
      } break;

      case SEQ_SONG_ACTION_SelMixerMap:
	SEQ_MIXER_Load(s->action_value);
	SEQ_MIXER_SendAll();
	++song_pos;
	again = 1;
	break;

    default:
      if( s->action >= SEQ_SONG_ACTION_Loop1 && s->action <= SEQ_SONG_ACTION_Loop16 ) {
	song_loop_ctr = 0;
	song_loop_ctr_max = s->action - SEQ_SONG_ACTION_Loop1;

	seq_pattern_t p;

	// TODO: implement prefetching until end of step!

	if( s->pattern_g1 < 0x80 ) {
	  p.ALL = 0;
	  p.pattern = s->pattern_g1;
	  p.bank = s->bank_g1;
	  SEQ_PATTERN_Change(0, p);
	}

	if( s->pattern_g2 < 0x80 ) {
	  p.ALL = 0;
	  p.pattern = s->pattern_g2;
	  p.bank = s->bank_g2;
	  SEQ_PATTERN_Change(1, p);
	}

	if( s->pattern_g3 < 0x80 ) {
	  p.ALL = 0;
	  p.pattern = s->pattern_g3;
	  p.bank = s->bank_g3;
	  SEQ_PATTERN_Change(2, p);
	}

	if( s->pattern_g4 < 0x80 ) {
	  p.ALL = 0;
	  p.pattern = s->pattern_g4;
	  p.bank = s->bank_g4;
	  SEQ_PATTERN_Change(3, p);
	}
      }
    }

  } while( again );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// fetches the next pos entry of a song
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_NextPos(void)
{
  if( song_loop_ctr < song_loop_ctr_max )
    ++song_loop_ctr;
  else {
    ++song_pos;
    return SEQ_SONG_FetchPos();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Loads a song
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Load(u32 song)
{
  s32 status;

  if( song >= SEQ_SONG_NUM )
    return -1; // invalid song number

  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_S_SongRead(song)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  // no auto-save required
  something_has_been_changed = 0;

  // new song number
  song_num = song;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Stores a song
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Save(u32 song)
{
  s32 status;

  if( song >= SEQ_SONG_NUM )
    return -1; // invalid song number

  if( !something_has_been_changed )
    return 0; // no store operation required

  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_S_SongWrite(song)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  return status;
}
