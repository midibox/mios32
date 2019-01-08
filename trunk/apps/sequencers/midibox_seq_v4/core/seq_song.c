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

#include "seq_ui.h"
#include "seq_lcd.h"

#include "seq_song.h"
#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_pattern.h"
#include "seq_mixer.h"
#include "seq_midi_in.h"

#include "seq_file.h"
#include "seq_file_s.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// should only be directly accessed by SEQ_FILE_S, remaining functions should
// use SEQ_SONG_StepEntryGet/Set
seq_song_step_t seq_song_steps[SEQ_SONG_NUM_STEPS];

char seq_song_name[21];

u8 seq_song_guide_track;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 song_num;
static u8 song_active; // switches between song/phrase mode
static u8 song_finished; // don't increment song position anymore

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
  song_finished = 0;

  seq_song_guide_track = 0;

  // initialise song steps
  int step;
  for(step=0; step<SEQ_SONG_NUM_STEPS; ++step)
    SEQ_SONG_StepEntryClear(step);

  // don't store these empty entries
  something_has_been_changed = 0;

  // reset the song sequencer
  SEQ_SONG_Reset(0);

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
// 0: phrase mode
// 1: song mode
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
// Get/Set guide track
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_GuideTrackGet(void)
{
  return seq_song_guide_track;
}

s32 SEQ_SONG_GuideTrackSet(u8 track)
{
  if( track > SEQ_CORE_NUM_TRACKS ) // 0..16, 0 disables guide track
    return -1; // invalid track

  seq_song_guide_track = track;

  // for auto-save operation
  something_has_been_changed = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Get/Set/Clear step entry of a song
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

s32 SEQ_SONG_StepEntryClear(u32 step)
{
  if( step >= SEQ_SONG_NUM_STEPS )
    return -1; // invalid step number

  seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[step];
  s->action = SEQ_SONG_ACTION_End;
  s->action_value = 0;
  s->pattern_g1 = 0x80;
  s->bank_g1 = 0;
  s->pattern_g2 = 0x80;
  s->bank_g2 = 1;
  s->pattern_g3 = 0x80;
  s->bank_g3 = 2;
  s->pattern_g4 = 0x80;
  s->bank_g4 = 3;

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
  song_finished = 0;
  song_pos = pos % SEQ_SONG_NUM_STEPS;
  return SEQ_SONG_FetchPos(0, 0);
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
s32 SEQ_SONG_Reset(u32 bpm_start)
{
  // TODO: take bpm_start into account!

  // reset song position and loop counter
  song_pos = (ui_page == SEQ_UI_PAGE_SONG) ? ui_song_edit_pos : 0;
  song_loop_ctr = 0;
  song_loop_ctr_max = 0;

  song_finished = 0;

  // if not in phrase mode: fetch new entries
  if( song_active ) {
    if( SEQ_SONG_FetchPos(1, 1) < 0 ) { // returns -1 if recursion counter reached max position
      // reset song positions and loop counters again
      song_pos = 0;
      song_loop_ctr = 0;
      song_loop_ctr_max = 0;
    }
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Help function for pattern switching
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_SONG_FetchHlp_PatternChange(u8 group, u8 pattern, u8 bank, u8 force_immediate_change)
{
  if( pattern < 0x80 ) {
    if( force_immediate_change || pattern != seq_pattern[group].pattern ) {
      seq_pattern_t p;
      p.ALL = 0;
      p.pattern = pattern;
      p.bank = bank;
      SEQ_PATTERN_Change(group, p, force_immediate_change);
    } else {
      // pattern not reloaded... we have to consider the RATOPC flag which is normally done in SEQ_PATTERN_Handler()
      if( seq_core_options.RATOPC ) {
	MIOS32_IRQ_Disable(); // must be atomic
	seq_core_state.reset_trkpos_req |= (0xf << (4*group));
	MIOS32_IRQ_Enable();
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// fetches the pos entries of a song
// returns -1 if recursion counter reached max position
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_FetchPos(u8 force_immediate_change, u8 dont_dump_mixer_map)
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
      case SEQ_SONG_ACTION_End:
#if 0
	if( song_active ) // not in phrase mode
	  SEQ_BPM_Stop();
#else
	song_finished = 1; // deactivate song incrementer
#endif
	break;

      case SEQ_SONG_ACTION_JmpPos:
	song_pos = s->action_value % SEQ_SONG_NUM_STEPS;
	again = 1;
	break;

      case SEQ_SONG_ACTION_JmpSong: {
	if( song_active ) { // not in phrase mode
	  u32 new_song_num = s->action_value % SEQ_SONG_NUM;

	  SEQ_SONG_Save(song_num);
	  SEQ_SONG_Load(new_song_num);
	  song_pos = 0;
	  again = 1;
	}
      } break;

      case SEQ_SONG_ACTION_SelMixerMap:
	SEQ_MIXER_Load(s->action_value);
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_MIXER_MAP, s->action_value, 0);
	if( !dont_dump_mixer_map )
	  SEQ_MIXER_SendAll();
	++song_pos;
	again = 1;
	break;

      case SEQ_SONG_ACTION_Tempo: {
	float bpm = (float)s->action_value;
	if( bpm < 25.0 )
	  bpm = 25.0;
	float ramp = (float)s->pattern_g1;
	SEQ_CORE_BPM_Update(bpm, ramp);
	++song_pos;
	again = 1;
      } break;

      case SEQ_SONG_ACTION_Mutes: {
	// access to seq_core_trk[] must be atomic!
	portENTER_CRITICAL();

	seq_core_trk_muted =
	  ((s->pattern_g1 & 0x0f) <<  0) |
	  ((s->pattern_g2 & 0x0f) <<  4) |
	  ((s->pattern_g3 & 0x0f) <<  8) |
	  ((s->pattern_g4 & 0x0f) << 12);

	portEXIT_CRITICAL();

	++song_pos;
	again = 1;
      } break;

      case SEQ_SONG_ACTION_GuideTrack: {
	if( s->action_value <= 16 )
	  seq_song_guide_track = s->action_value;
	++song_pos;
	again = 1;
      } break;

      case SEQ_SONG_ACTION_UnmuteAll: {
	// access to seq_core_trk[] must be atomic!
	portENTER_CRITICAL();

	seq_core_trk_muted = 0x0000;

	{
	  seq_core_trk_t *t = &seq_core_trk[0];
	  u8 track;
	  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t) {
	    t->layer_muted = 0x0000;
	  }
	}

	portEXIT_CRITICAL();

	++song_pos;
	again = 1;
      } break;

      default:
	if( s->action >= SEQ_SONG_ACTION_Loop1 && s->action <= SEQ_SONG_ACTION_Loop16 ) {
	  song_loop_ctr = 0;
	  song_loop_ctr_max = s->action - SEQ_SONG_ACTION_Loop1;

	  // TODO: implement prefetching until end of step!
	  
	  SEQ_SONG_FetchHlp_PatternChange(0, s->pattern_g1, s->bank_g1, force_immediate_change);
	  SEQ_SONG_FetchHlp_PatternChange(1, s->pattern_g2, s->bank_g2, force_immediate_change);
	  SEQ_SONG_FetchHlp_PatternChange(2, s->pattern_g3, s->bank_g3, force_immediate_change);
	  SEQ_SONG_FetchHlp_PatternChange(3, s->pattern_g4, s->bank_g4, force_immediate_change);
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
  if( song_finished )
    return 0;

  if( song_loop_ctr < song_loop_ctr_max )
    ++song_loop_ctr;
  else {
    ++song_pos;

    SEQ_SONG_FetchPos(0, 0);

#ifndef MBSEQV4L
    // correct the song position if follow mode is active
    if( seq_core_state.FOLLOW && SEQ_SONG_ActiveGet() )
      SEQ_UI_SONG_EditPosSet(song_pos);
#endif

    // update display immediately
    seq_ui_display_update_req = 1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// fetches the previous pos entry of a song
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_PrevPos(void)
{
  song_finished = 0;

  if( song_loop_ctr )
    --song_loop_ctr;
  else {
    u8 reinit_loop_ctr = 1;

    if( song_pos ) {
      --song_pos;

      seq_song_step_t *s = (seq_song_step_t *)&seq_song_steps[song_pos];
      while( song_pos && // on certain actions we should go back one additional step
	     (s->action == SEQ_SONG_ACTION_SelMixerMap ||
	      s->action == SEQ_SONG_ACTION_Tempo ||
	      s->action == SEQ_SONG_ACTION_Mutes ||
	      s->action == SEQ_SONG_ACTION_UnmuteAll ||
	      s->action == SEQ_SONG_ACTION_GuideTrack ) ) {
	--song_pos;
	--s;
      }
    } else if( !song_loop_ctr )
      reinit_loop_ctr = 0; // don't re-init loop counter if we reached the very first song step

    SEQ_SONG_FetchPos(0, 0);

    if( reinit_loop_ctr )
      song_loop_ctr = song_loop_ctr_max;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// called by the UI to handle the "Fwd" button
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Fwd(void)
{
  if( ui_page == SEQ_UI_PAGE_SONG ) {
    song_pos = ui_song_edit_pos;
    song_loop_ctr = 0;
    SEQ_SONG_NextPos();
    if( ui_song_edit_pos != song_pos )
      ui_song_edit_pos = song_pos;
    else {
      // increment if possible
      if( song_pos < SEQ_SONG_NUM_STEPS ) {
	++song_pos;
	SEQ_SONG_FetchPos(0, 0);
	ui_song_edit_pos = song_pos;

	// update display immediately
	seq_ui_display_update_req = 1;
      }
    }
  } else {
    u32 bpm_tick = SEQ_BPM_TickGet();
    u32 ticks_per_pattern = ((u32)seq_core_steps_per_pattern+1) * (SEQ_BPM_PPQN_Get()/4);
    u32 measure = bpm_tick / ticks_per_pattern;
    u32 next_bpm_tick = (measure+1) * ticks_per_pattern;

    SEQ_CORE_Reset(next_bpm_tick);
    SEQ_SONG_NextPos();
    SEQ_BPM_TickSet(next_bpm_tick);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// called by the UI to handle the "Rew" button
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_SONG_Rew(void)
{
  if( ui_page == SEQ_UI_PAGE_SONG ) {
    song_pos = ui_song_edit_pos;
    song_loop_ctr = 0;
    SEQ_SONG_PrevPos();
    ui_song_edit_pos = song_pos;
  } else {
    u32 bpm_tick = SEQ_BPM_TickGet();
    u32 ticks_per_pattern = ((u32)seq_core_steps_per_pattern+1) * (SEQ_BPM_PPQN_Get()/4);
    u32 measure = bpm_tick / ticks_per_pattern;
    u32 next_bpm_tick = measure ? ((measure-1) * ticks_per_pattern) : 0;

    SEQ_CORE_Reset(next_bpm_tick);
    SEQ_SONG_PrevPos();
    SEQ_BPM_TickSet(next_bpm_tick);
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
  if( (status=SEQ_FILE_S_SongWrite(seq_file_session_name, song, 0)) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  // no auto-save required anymore?
  if( status >= 0 )
    something_has_been_changed = 0;

  return status;
}
