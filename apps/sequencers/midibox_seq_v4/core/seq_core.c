// $Id$
/*
 * Sequencer Core Routines
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
#include <string.h>
#include <seq_bpm.h>
#include <seq_midi_out.h>

#include "tasks.h"

#include "seq_core.h"
#include "seq_song.h"
#include "seq_random.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_scale.h"
#include "seq_groove.h"
#include "seq_humanize.h"
#include "seq_robotize.h"
#include "seq_morph.h"
#include "seq_lfo.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"
#include "seq_random.h"
#include "seq_record.h"
#include "seq_live.h"
#include "seq_midply.h"
#include "seq_midexp.h"
#include "seq_midimp.h"
#include "seq_cv.h"
#include "seq_statistics.h"
#include "seq_ui.h"



/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// set this to 1 if performance of clock handler should be measured with a scope
// (LED toggling in APP_Background() has to be disabled!)
// set this to 2 to visualize forward delay during pattern changes
#define LED_PERFORMANCE_MEASURING 0

// same for measuring with the stopwatch
// value is visible in menu (-> press exit button)
// value is visible in INFO->System page (-> press exit button, go to last item)
#define STOPWATCH_PERFORMANCE_MEASURING 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_CORE_ResetTrkPos(u8 track, seq_core_trk_t *t, seq_cc_trk_t *tcc);
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, u8 no_progression, u8 reverse);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_core_options_t seq_core_options;
u8 seq_core_steps_per_measure;
u8 seq_core_steps_per_pattern;

u16 seq_core_trk_muted;
u16 seq_core_trk_synched_mute;
u16 seq_core_trk_synched_unmute;
seq_core_slaveclk_mute_t seq_core_slaveclk_mute;
u16 seq_core_trk_soloed;

u8 seq_core_step_update_req;

u8 seq_core_global_scale;
u8 seq_core_global_scale_root;
u8 seq_core_global_scale_root_selection;
u8 seq_core_keyb_scale_root;

u8 seq_core_global_transpose_enabled;

u8 seq_core_bpm_preset_num;
float seq_core_bpm_preset_tempo[SEQ_CORE_NUM_BPM_PRESETS];
float seq_core_bpm_preset_ramp[SEQ_CORE_NUM_BPM_PRESETS];

u8 seq_core_din_sync_pulse_ctr;

mios32_midi_port_t seq_core_metronome_port;
u8 seq_core_metronome_chn;
u8 seq_core_metronome_note_m;
u8 seq_core_metronome_note_b;

mios32_midi_port_t seq_core_shadow_out_port;
u8 seq_core_shadow_out_chn;

seq_core_state_t seq_core_state;
seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];

seq_core_loop_mode_t seq_core_glb_loop_mode;
u8 seq_core_glb_loop_offset;
u8 seq_core_glb_loop_steps;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 bpm_tick_prefetch_req;
static u32 bpm_tick_prefetched;

static float seq_core_bpm_target;
static float seq_core_bpm_sweep_inc;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
// \param mode if 0: clear all parameters, if 1: don't clear global parameters which are stored in the MBSEQ_GC.V4 file
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Init(u32 mode)
{
  int i;

  seq_core_trk_muted = 0;
  seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
  seq_core_trk_soloed = 0;
  seq_core_options.ALL = 0;
  if( mode == 0 ) {
    seq_core_options.INIT_CC = 0x80; // off
    seq_core_options.INIT_WITH_TRIGGERS = 1;
    seq_core_options.PASTE_CLR_ALL = 1;
    seq_core_options.PATTERN_MIXER_MAP_COUPLING = 0;
    seq_core_options.MIXER_LIVE_SEND = 1;
  }
  seq_core_steps_per_measure = 16-1;
  seq_core_steps_per_pattern = 16-1;
  seq_core_global_scale = 0;
  seq_core_global_scale_root_selection = 0; // from keyboard
  seq_core_keyb_scale_root = 0; // taken if enabled in OPT menu
  seq_core_global_transpose_enabled = 0;
  seq_core_din_sync_pulse_ctr = 0; // used to generate a 1 mS pulse

  if( mode == 0 ) {
    seq_core_metronome_port = DEFAULT;
    seq_core_metronome_chn = 10;
    seq_core_metronome_note_m = 0x25; // C#1
    seq_core_metronome_note_b = 0x25; // C#1
  }

  if( mode == 0 ) {
    seq_core_shadow_out_port = DEFAULT;
    seq_core_shadow_out_chn = 0; // means: off
  }

  seq_core_bpm_preset_num = 13; // 140.0
  for(i=0; i<SEQ_CORE_NUM_BPM_PRESETS; ++i) {
    seq_core_bpm_preset_tempo[i] = 75.0 + 5.0*i;
    seq_core_bpm_preset_ramp[i] = 0.0;
  }
  seq_core_bpm_sweep_inc = 0.0;

  seq_core_state.ALL = 0;

  seq_core_glb_loop_mode = SEQ_CORE_LOOP_MODE_ALL_TRACKS_VIEW;
  seq_core_glb_loop_offset = 0;
  seq_core_glb_loop_steps = 16-1;

  seq_core_step_update_req = 0;

  // set initial seed of random generator
  SEQ_RANDOM_Gen(0xdeadbabe);

  // reset layers
  SEQ_LAYER_Init(0);

  // reset patterns
  SEQ_PATTERN_Init(0);

  // reset force-to-scale module
  SEQ_SCALE_Init(0);

  // reset groove module
  SEQ_GROOVE_Init(0);

  // reset morph module
  SEQ_MORPH_Init(0);

  // reset humanizer module
  SEQ_HUMANIZE_Init(0);


  // reset robotizer module
  SEQ_ROBOTIZE_Init(0);

  // reset LFO module
  SEQ_LFO_Init(0);

  // reset song module
  SEQ_SONG_Init(0);

  // reset live play module
  SEQ_LIVE_Init(0);

  // reset record module
  SEQ_RECORD_Init(0);

  // init MIDI file player/exporter/importer
  SEQ_MIDPLY_Init(0);
  SEQ_MIDEXP_Init(0);
  SEQ_MIDIMP_Init(0);

  // clear registers which are not reset by SEQ_CORE_Reset()
  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t) {

    // if track name only contains spaces, the UI will print 
    // the track number instead of an empty message
    // this ensures highest flexibility (e.g. any track can 
    // play patterns normaly assigned for other tracks w/o inconsistent messages)
    // -> see SEQ_LCD_PrintTrackName()
    int i;
    for(i=0; i<80; ++i)
      t->name[i] = ' ';
    t->name[80] = 0;

    // clear glide note storage
    for(i=0; i<4; ++i)
      t->glide_notes[i] = 0;

    // don't select sections
    t->play_section = 0;
  }

  // reset sequencer
  SEQ_CORE_Reset(0);

  // reset MIDI player
  SEQ_MIDPLY_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

  SEQ_BPM_PPQN_Set(384);
  SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], 0.0);

#if STOPWATCH_PERFORMANCE_MEASURING
  SEQ_STATISTICS_StopwatchInit();
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function schedules a MIDI event by considering the "normal" and "Fx"
// MIDI port
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_ScheduleEvent(u8 track, seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t midi_package, seq_midi_out_event_type_t event_type, u32 timestamp, u32 len, u8 is_echo, seq_robotize_flags_t robotize_flags)
{
  s32 status = 0;
  mios32_midi_port_t fx_midi_port = tcc->fx_midi_port ? tcc->fx_midi_port : tcc->midi_port;

  u8 shadow_enabled = seq_core_shadow_out_chn && SEQ_UI_VisibleTrackGet() == track;

  //check that there are more than 0 additional channels, that it's not just one channel and FX starting on current channel, check disable flag, check for robotizer
  if( ! ( tcc->fx_midi_num_chn & 0x3f ) || ( ( tcc->fx_midi_num_chn & 0x3f ) == 1 && tcc->fx_midi_chn == midi_package.chn  ) || ( ( tcc->fx_midi_num_chn & 0x40 ) && !robotize_flags.DUPLICATE ) ) {
    status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, event_type, timestamp, len);

    if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
      midi_package.velocity = 0;
      status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
    }
  } else {
    if( event_type == SEQ_MIDI_OUT_OnEvent || event_type == SEQ_MIDI_OUT_OnOffEvent ) {
      switch( tcc->fx_midi_mode.beh ) {
      case SEQ_CORE_FX_MIDI_MODE_BEH_Alternate:
      case SEQ_CORE_FX_MIDI_MODE_BEH_AlternateSynchedEcho: {
	// forward to next channel
	if( (tcc->fx_midi_mode.beh == SEQ_CORE_FX_MIDI_MODE_BEH_AlternateSynchedEcho && !is_echo) ||
	  ++t->fx_midi_ctr > ( tcc->fx_midi_num_chn & 0x3f ) )
	  t->fx_midi_ctr = 0;

	mios32_midi_port_t midi_port;
	if( t->fx_midi_ctr == 0 ) {
	  midi_port = tcc->midi_port;
	} else {
	  midi_port = fx_midi_port;
	  midi_package.chn = (( tcc->fx_midi_num_chn & 0x3f ) + (t->fx_midi_ctr-1)) % 16;
	}

	status |= SEQ_MIDI_OUT_Send(midi_port, midi_package, event_type, timestamp, len);

	if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
	  midi_package.velocity = 0;
	  status |= SEQ_MIDI_OUT_Send(midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
	}
      } break;
	
      case SEQ_CORE_FX_MIDI_MODE_BEH_Random: {
	// select random channel
	int ix = SEQ_RANDOM_Gen_Range(0, ( tcc->fx_midi_num_chn & 0x3f ));
	mios32_midi_port_t midi_port;
	if( ix == 0 ) {
	  midi_port = tcc->midi_port;
	} else {
	  midi_port = fx_midi_port;
	  midi_package.chn = (( tcc->fx_midi_num_chn & 0x3f ) + ix-1) % 16;
	}

	status |= SEQ_MIDI_OUT_Send(midi_port, midi_package, event_type, timestamp, len);

	if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
	  midi_package.velocity = 0;
	  status |= SEQ_MIDI_OUT_Send(midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
	}
      } break;

      default: { // && SEQ_CORE_FX_MIDI_MODE_BEH_Forward
	// forward to all channels

	// original channel
	status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, event_type, timestamp, len);

	// all other channels
	int ix;
	for(ix=0; ix < ( tcc->fx_midi_num_chn & 0x3f ); ++ix) {
	  midi_package.chn = (tcc->fx_midi_chn + ix) % 16;
	  status |= SEQ_MIDI_OUT_Send(fx_midi_port, midi_package, event_type, timestamp, len);
	}

	if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
	  midi_package.velocity = 0;

	  midi_package.chn = tcc->midi_chn % 16;
	  status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);

	  for(ix=0; ix< ( tcc->fx_midi_num_chn & 0x3f ); ++ix) {
	    midi_package.chn = (tcc->fx_midi_chn + ix) % 16;
	    status |= SEQ_MIDI_OUT_Send(fx_midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
	  }
	}
      } break;


      }
    } else {
      // original channel
      status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, event_type, timestamp, len);

      if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
	midi_package.velocity = 0;
	status |= SEQ_MIDI_OUT_Send(tcc->midi_port, midi_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
      }

      if( tcc->fx_midi_mode.fwd_non_notes ) {
	// all other channels
	int ix;
	for(ix=0; ix < ( tcc->fx_midi_num_chn & 0x3f ); ++ix) {
	  midi_package.chn = (tcc->fx_midi_chn + ix) % 16;
	  status |= SEQ_MIDI_OUT_Send(fx_midi_port, midi_package, event_type, timestamp, len);
	}
      }
    }
  }

  // duplicate to shadow out?
  if( shadow_enabled ) {
    mios32_midi_package_t shadow_package = midi_package;
    shadow_package.chn = seq_core_shadow_out_chn - 1;

    // duplicate to shadow channel
    if( shadow_enabled ) {
      status |= SEQ_MIDI_OUT_Send(seq_core_shadow_out_port, shadow_package, event_type, timestamp, len);

      if( event_type == SEQ_MIDI_OUT_OnEvent ) { // schedule off event at same port
	shadow_package.velocity = 0;
	status |= SEQ_MIDI_OUT_Send(seq_core_shadow_out_port, shadow_package, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);
      }
    }
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// this sequencer handler is called periodically to check for new requests
// from BPM generator
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Handler(void)
{
  // handle requests

  u8 num_loops = 0;
  u8 again = 0;
  do {
    ++num_loops;

    // note: don't remove any request check - clocks won't be propagated
    // as long as any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xfc, 0);
      SEQ_CORE_PlayOffEvents();
      SEQ_MIDPLY_PlayOffEvents();

      int track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	SEQ_RECORD_Reset(track);
      }
    }

    if( SEQ_BPM_ChkReqCont() ) {
      // release slave mute
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;

      // update delays
      SEQ_MIDI_PORT_ClkDelayUpdateAll();

      // send continue event
      SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xfb, 0);

      // release pause mode
      ui_seq_pause = 0;
    }

    if( SEQ_BPM_ChkReqStart() ) {
      // release slave mute
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;

      // update delays
      SEQ_MIDI_PORT_ClkDelayUpdateAll();

      // send start event and reset sequencer
      SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xfa, 0);
      SEQ_SONG_Reset(0);
      SEQ_CORE_Reset(0);
      SEQ_MIDPLY_Reset();

      // song page: start song at current edit position
      if( ui_page == SEQ_UI_PAGE_SONG ) {
	SEQ_SONG_PosSet(ui_song_edit_pos);
      }
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      // release slave mute
      seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;

      // update delays
      SEQ_MIDI_PORT_ClkDelayUpdateAll();

      // new position
      u32 new_tick = new_song_pos * (SEQ_BPM_PPQN_Get() / 4);
      SEQ_CORE_Reset(new_tick);
      SEQ_SONG_Reset(new_tick);

#if 0
      // fast forward to new song position
      if( new_tick ) {
	u32 bpm_tick;
	for(bpm_tick=0; bpm_tick<=new_tick; bpm_tick+=24) {
	  SEQ_BPM_TickSet(bpm_tick);
	  SEQ_CORE_Tick(bpm_tick, -1, 1); // mute all non-loopback tracks
	}
	SEQ_BPM_TickSet(new_tick);
      }
#endif
      SEQ_MIDPLY_SongPos(new_song_pos, 1);
    }

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      // check all requests again after execution of this part
      again = 1;

      // it's possible to forward the sequencer on pattern changes
      // in this case bpm_tick_prefetch_req is > bpm_tick
      // in all other cases, we only generate a single tick (realtime play)
      u32 add_bpm_ticks = 0;
      if( bpm_tick_prefetch_req > bpm_tick ) {
	add_bpm_ticks = bpm_tick_prefetch_req - bpm_tick;
      }
      // invalidate request before a new one will be generated (e.g. via SEQ_SONG_NextPos())
      bpm_tick_prefetch_req = 0;

      // consider negative delay offsets: preload the appr. number of ticks
      s8 max_negative_offset = SEQ_MIDI_PORT_TickDelayMaxNegativeOffset();	
      if( max_negative_offset < 0 ) {
	s32 offset = -max_negative_offset + 3; // +3 margin
	add_bpm_ticks += offset;
      }

      // remove ticks which have already been processed before
      u32 bpm_tick_target = bpm_tick + add_bpm_ticks;
      if( !seq_core_state.FIRST_CLK && bpm_tick <= bpm_tick_prefetched ) {
	bpm_tick = bpm_tick_prefetched + 1;
      }

      // processing remaining ticks
      for(; bpm_tick<=bpm_tick_target; ++bpm_tick) {
	bpm_tick_prefetched = bpm_tick;

#if LED_PERFORMANCE_MEASURING == 1
	MIOS32_BOARD_LED_Set(0x00000001, 1);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 1
	SEQ_STATISTICS_StopwatchReset();
#endif

	// generate MIDI events
	SEQ_CORE_Tick(bpm_tick, -1, 0);
	SEQ_MIDPLY_Tick(bpm_tick);

#if LED_PERFORMANCE_MEASURING == 1
	MIOS32_BOARD_LED_Set(0x00000001, 0);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 1
	SEQ_STATISTICS_StopwatchCapture();
#endif

	// load new pattern/song step if reference step reached measure
	// (this code is outside SEQ_CORE_Tick() to save stack space!)
	if( (bpm_tick % 96) == 20 ) {
	  if( SEQ_SONG_ActiveGet() ) {
	    // to handle the case as described under http://midibox.org/forums/topic/19774-question-about-expected-behaviour-in-song-mode/
	    // seq_core_steps_per_measure was lower than seq_core_steps_per_pattern
	    u32 song_switch_step = (seq_core_steps_per_measure < seq_core_steps_per_pattern) ? seq_core_steps_per_measure : seq_core_steps_per_pattern;
	    if( ( seq_song_guide_track && seq_song_guide_track <= SEQ_CORE_NUM_TRACKS &&
		  seq_core_state.ref_step_song == seq_cc_trk[seq_song_guide_track-1].length) ||
		(!seq_song_guide_track && seq_core_state.ref_step_song == song_switch_step) ) {
	      
	      if( seq_song_guide_track ) {
		// request synch-to-measure for all tracks
		SEQ_CORE_ManualSynchToMeasure(0xffff);
		
		// corner case: we will load new tracks and the length of the guide track could change
		// in order to ensure that the reference step jumps back to 0, we've to force this here:
		seq_core_state.FORCE_REF_STEP_RESET = 1;
	      }
	      
	      SEQ_SONG_NextPos();
	    }
	  } else {
	    if( seq_core_options.SYNCHED_PATTERN_CHANGE &&
		seq_core_state.ref_step_pattern == seq_core_steps_per_pattern ) {
	      SEQ_PATTERN_Handler();
	    }
	  }
	}
      }
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // clear sustain/stretch flags
  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t) {
    int i;

    t->state.SUSTAINED = 0;
    t->state.STRETCHED_GL = 0;
    for(i=0; i<4; ++i)
      t->glide_notes[i] = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Reset(u32 bpm_start)
{
  ui_seq_pause = 0;
  seq_core_state.FIRST_CLK = 1;

  // reset latched PB/CC values
  SEQ_LAYER_ResetLatchedValues();

  int track;
  seq_core_trk_t *t = &seq_core_trk[0];
  seq_cc_trk_t *tcc = &seq_cc_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t, ++tcc) {
    // clear all states (except for recording)
    {
      u8 rec_dont_overwrite_next_step = t->state.REC_DONT_OVERWRITE_NEXT_STEP;
      t->state.ALL = 0;
      t->state.REC_DONT_OVERWRITE_NEXT_STEP = rec_dont_overwrite_next_step;
    }
    SEQ_CORE_ResetTrkPos(track, t, tcc);

    t->bar = 0;
    t->layer_muted_from_midi = 0;
    t->layer_muted_from_midi_next = 0;
    t->lfo_cc_muted_from_midi = 0;
    t->lfo_cc_muted_from_midi_next = 0;

    // add track offset depending on start position
    if( bpm_start ) {
#if 0
      u32 step_length = ((tcc->clkdiv.value+1) * (tcc->clkdiv.TRIPLETS ? 4 : 6));
#else
      // leads to bad result with Logic Audio: it starts one step earlier and assumes 16th steps!
      u32 step_length = 96;
#endif
      u32 pos_step = (u8)((bpm_start / step_length) % ((u32)tcc->length+1));
      u32 pos_bar  = (u8)((bpm_start / step_length) / ((u32)tcc->length+1));

      // next part depends on forward/backward direction
      if( tcc->dir_mode == SEQ_CORE_TRKDIR_Backward ) {
	t->step = tcc->length - pos_step;
      } else {
	t->step = pos_step;
      }

      t->bar = pos_bar;
    }
  }

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_CORE_PlayOffEvents();

  // set BPM tick
  SEQ_BPM_TickSet(bpm_start);

  // cancel prefetch requests/counter
  bpm_tick_prefetch_req = 0;
  bpm_tick_prefetched = bpm_start;

  // cancel stop and set step request
  seq_core_state.MANUAL_TRIGGER_STOP_REQ = 0;

  // reset reference step
  seq_core_state.ref_step = (u16)((bpm_start / 96) % ((u32)seq_core_steps_per_measure+1));
  seq_core_state.ref_step_pattern = seq_core_state.ref_step;
  if( seq_song_guide_track ) {
    seq_core_state.ref_step_song = (u16)((bpm_start / 96) % ((u32)seq_cc_trk[seq_song_guide_track-1].length+1));
  } else {
    seq_core_state.ref_step_song = seq_core_state.ref_step;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
// if "export_track" is -1, all tracks will be played
// if "export_track" is between 0 and 15, only the given track + all loopback
//   tracks will be played (for MIDI file export)
// if "mute_nonloopback_tracks" is set, the "normal" tracks won't be played
// this option is used for the "fast forward" function on song position changes
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Tick(u32 bpm_tick, s8 export_track, u8 mute_nonloopback_tracks)
{
  // get MIDI File play mode (if set to SEQ_MIDPLY_MODE_Exclusive, all tracks will be muted)
  u8 midply_solo = SEQ_MIDPLY_RunModeGet() != 0 && SEQ_MIDPLY_ModeGet() == SEQ_MIDPLY_MODE_Exclusive; 

  // increment reference step on each 16th note
  // set request flag on overrun (tracks can synch to measure)
  u8 synch_to_measure_req = 0;
  if( (bpm_tick % (384/4)) == 0 ) {
    seq_core_trk_t *t = &seq_core_trk[0];
    seq_cc_trk_t *tcc = &seq_cc_trk[0];
    u8 track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t, ++tcc) {
      if( seq_core_state.reset_trkpos_req & (1 << track) ) {
	SEQ_CORE_ResetTrkPos(track, t, tcc);
	++t->bar;
      }

      // NEW: temporary layer mutes on incoming MIDI
      // take over _next mutes
      int num_steps = (seq_core_options.LIVE_LAYER_MUTE_STEPS < 2) ? 1 : (seq_core_options.LIVE_LAYER_MUTE_STEPS-2+1);
      if( (bpm_tick % (num_steps*(384/4))) == 0 ) {
	// layer mutes
	t->layer_muted_from_midi = t->layer_muted_from_midi_next;
	t->layer_muted_from_midi_next = 0;

	// the same for the LFO CC
	t->lfo_cc_muted_from_midi = t->lfo_cc_muted_from_midi_next;
	t->lfo_cc_muted_from_midi_next = 0;
      }
    }
    seq_core_state.reset_trkpos_req = 0;

    if( seq_core_state.FIRST_CLK || seq_core_state.FORCE_REF_STEP_RESET ) {
      seq_core_state.FORCE_REF_STEP_RESET = 0;
      synch_to_measure_req = 1;
      seq_core_state.ref_step = 0;
      seq_core_state.ref_step_pattern = 0;
      seq_core_state.ref_step_song = 0;
    } else {
      if( ++seq_core_state.ref_step > seq_core_steps_per_measure ) {
	seq_core_state.ref_step = 0;
      }

      if( ++seq_core_state.ref_step_pattern > seq_core_steps_per_pattern ) {
	seq_core_state.ref_step_pattern = 0;
      }

      if( seq_song_guide_track ) {
	if( ++seq_core_state.ref_step_song > seq_cc_trk[seq_song_guide_track-1].length ) {
	  seq_core_state.ref_step_song = 0;
	}
      } else {
	seq_core_state.ref_step_song = seq_core_state.ref_step;
      }

      if( SEQ_SONG_ActiveGet() ) {
	if( seq_core_state.ref_step_song == 0 )
	  synch_to_measure_req = 1;
      } else {
	if( seq_core_state.ref_step == 0 )
	  synch_to_measure_req = 1;
      }
    }
  }

  // disable slave clock mute if not in slave mode anymore
  // or start of if measure is reached and OffOnNextMeasure has been requested
  u8 is_master = SEQ_BPM_IsMaster();
  if( is_master ||
      (synch_to_measure_req && (seq_core_slaveclk_mute == SEQ_CORE_SLAVECLK_MUTE_OffOnNextMeasure)) ) {
    // enable ports
    seq_core_slaveclk_mute = SEQ_CORE_SLAVECLK_MUTE_Off;
    if( !is_master ) {
      // release pause mode
      ui_seq_pause = 0;
      // TK: this makes sense! Request synch-to-measure for all tracks so that they restart properly
      SEQ_CORE_ManualSynchToMeasure(0xffff);
    }
  }

  // if no export and no mute:
  if( export_track == -1 && !mute_nonloopback_tracks ) {
    // send FA if external restart has been requested
    if( seq_core_state.EXT_RESTART_REQ && synch_to_measure_req ) {
      seq_core_state.EXT_RESTART_REQ = 0; // remove request
      seq_ui_display_update_req = 1; // request display update
      SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xfa, bpm_tick);
    }

    // send MIDI clock on each 16th tick (since we are working at 384ppqn)
    if( (bpm_tick % 16) == 0 )
      SEQ_MIDI_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);

#if !defined(MIOS32_DONT_USE_AOUT)
    // trigger DIN Sync clock with a special event (0xf9 normaly used for "MIDI tick")
    // SEQ_MIDI_PORT_NotifyMIDITx filters it before it will be forwarded to physical ports
    {
      int clkout;

      mios32_midi_package_t p;
      p.ALL = 0;
      p.type = 0x5; // Single-byte system common message
      p.evnt0 = 0xf9;

      u16 *clk_divider = (u16 *)&seq_cv_clkout_divider[0];
      for(clkout=0; clkout<SEQ_CV_NUM_CLKOUT; ++clkout, ++clk_divider) {
	if( *clk_divider && (bpm_tick % *clk_divider) == 0 ) {
	  p.evnt1 = clkout; // Transfers the Clock Output
	  SEQ_MIDI_OUT_Send(0xff, p, SEQ_MIDI_OUT_ClkEvent, bpm_tick, 0);
	}
      }
    }
#endif

    // send metronome tick on each beat if enabled
    if( seq_core_state.METRONOME && seq_core_metronome_chn && (bpm_tick % 96) == 0 && (seq_core_state.ref_step % 4) == 0 ) {
      mios32_midi_package_t p;

      p.type     = NoteOn;
      p.cable    = 15; // use tag of track #16 - unfortunately more than 16 tags are not supported
      p.event    = NoteOn;
      p.chn      = seq_core_metronome_chn-1;
      p.note     = seq_core_metronome_note_b;
      p.velocity = 96;
      u16 len = 20; // ca. 25% of a 16th

      if( seq_core_state.ref_step == 0 ) {
	if( seq_core_metronome_note_m )
	  p.note = seq_core_metronome_note_m; // if this note isn't defined, use B note instead
	p.velocity = 127;
      }

      if( p.note )
	SEQ_MIDI_OUT_Send(seq_core_metronome_port, p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick, len);
    }
  }

  // delayed mute
  if( synch_to_measure_req ) {
    // the priority handling for each individual track ensures
    // that mute/unmute will be done depending on the current mute state
    // unmute has higher priority than mute
    seq_core_trk_muted |= (seq_core_trk_synched_mute & ~seq_core_trk_muted);
    seq_core_trk_synched_mute = 0;

    seq_core_trk_muted &= ~(seq_core_trk_synched_unmute & seq_core_trk_muted);
    seq_core_trk_synched_unmute = 0;
  }

  // process all tracks
  // first the loopback port Bus1, thereafter parameters sent to common MIDI ports
  int round;
  for(round=0; round<2; ++round) {
    seq_core_trk_t *t = &seq_core_trk[0];
    seq_cc_trk_t *tcc = &seq_cc_trk[0];
    int track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++t, ++tcc, ++track) {

      seq_robotize_flags_t robotize_flags;
      robotize_flags.ALL = 0;

      // round 0: loopback port Bus1, round 1: remaining ports
      u8 loopback_port = (tcc->midi_port & 0xf0) == 0xf0;
      if( (!round && !loopback_port) || (round && loopback_port) )
	continue;

      // for MIDI file export: (export_track != -1): only given track + all loopback tracks will be played
      if( round && export_track != -1 && export_track != track )
	continue;

      // recording enabled for this track?
#ifndef MBSEQV4L
      u8 track_record_enabled = (seq_record_state.ENABLED && (SEQ_UI_VisibleTrackGet() == track)) ? 1 : 0;
#else
      u8 track_record_enabled = (seq_record_state.ENABLED && (seq_record_state.ARMED_TRACKS & (1 << track)) != 0) ? 1 : 0;
#endif

      // handle LFO effect
      SEQ_LFO_HandleTrk(track, bpm_tick);

      // send LFO CC (if enabled and not muted)
      if( !(seq_core_trk_muted & (1 << track)) && !seq_core_slaveclk_mute && !t->lfo_cc_muted_from_midi ) {
	mios32_midi_package_t p;
	if( SEQ_LFO_FastCC_Event(track, bpm_tick, &p, 0) > 0 ) {
	  if( loopback_port )
	    SEQ_MIDI_IN_BusReceive(tcc->midi_port & 0x0f, p, 1); // forward to MIDI IN handler immediately
	  else
	    SEQ_CORE_ScheduleEvent(track, t, tcc, p, SEQ_MIDI_OUT_CCEvent, bpm_tick, 0, 0, robotize_flags);
	}
      }

      // sustained note: play off event if sustain mode has been disabled and no stretched gatelength
      if( t->state.SUSTAINED && (t->state.CANCEL_SUSTAIN_REQ || (!tcc->trkmode_flags.SUSTAIN && !t->state.ROBOSUSTAINED && !t->state.STRETCHED_GL)) ) {
	int i;

	// important: play Note Off before new Note On to avoid that glide is triggered on the synth
	SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick ? (bpm_tick-1) : 0,
				track_record_enabled ? seq_record_played_notes : NULL);
	// clear state flag and note storage
	t->state.SUSTAINED = 0;
	for(i=0; i<4; ++i)
	  t->glide_notes[i] = 0;
      }
      t->state.CANCEL_SUSTAIN_REQ = 0;

      // if "synch to measure" flag set: reset track if master has reached the selected number of steps
      // MEMO: we could also provide the option to synch to another track
      if( synch_to_measure_req && (tcc->clkdiv.SYNCH_TO_MEASURE || t->state.SYNC_MEASURE) ) {
        SEQ_CORE_ResetTrkPos(track, t, tcc);
	++t->bar;
      }

      u8 mute_this_step = 0;
      u8 next_step_event = t->state.FIRST_CLK || bpm_tick >= t->timestamp_next_step;

      // step trigger: only play the new step if transposer has a note
      if( tcc->trkmode_flags.STEP_TRG ) {
	if( !t->state.TRIGGER_NEXT_STEP_REQ ) {
	  next_step_event = 0;
	}
      }
      t->state.TRIGGER_NEXT_STEP_REQ = 0;

      if( next_step_event ) {

	{
	  // calculate step length
	  u16 step_length_pre = ((tcc->clkdiv.value+1) * (tcc->clkdiv.TRIPLETS ? 4 : 6));
	  t->step_length = step_length_pre;

	  // set timestamp of next step w/o groove delay (reference timestamp)
	  if( t->state.FIRST_CLK )
	    t->timestamp_next_step_ref = bpm_tick + t->step_length;
	  else
	    t->timestamp_next_step_ref += t->step_length;

	  // increment step if not in arpeggiator mode or arp position == 0
	  u8 inc_step = tcc->playmode != SEQ_CORE_TRKMODE_Arpeggiator || !t->arp_pos;

	  // wrap step position around length - especially required for "section selection" later,
	  // which can set t->step beyond tcc->length+1
	  u8 prev_step = (u8)((int)t->step % ((int)tcc->length + 1));
	  t->step = prev_step; // store back wrapped step position

	  u8 skip_ctr = 0;
	  do {
	    if( t->state.MANUAL_STEP_REQ ) {
	      // manual step requested
	      t->state.MANUAL_STEP_REQ = 0;
	      t->step = t->manual_step;
	      t->step_saved = t->manual_step;
	      t->arp_pos = 0;
	    } else if( tcc->clkdiv.MANUAL ) {
	      // if clkdiv MANUAL mode: step was not requested, skip it!
	      mute_this_step = 1;
	    } else {
	      // determine next step depending on direction mode
	      if( !t->state.FIRST_CLK && inc_step )
		SEQ_CORE_NextStep(t, tcc, 0, 0); // 0, 0=with progression, not reverse
	      else {
		// ensure that position reset request is cleared
		t->state.POS_RESET = 0;
	      }
	    }
	    
	    // clear "first clock" flag (on following clock ticks we can continue as usual)
	    t->state.FIRST_CLK = 0;
  
	    // if skip flag set for this flag: try again
	    if( SEQ_TRG_SkipGet(track, t->step, 0) )
	      ++skip_ctr;
	    else
	      break;
	    
	  } while( skip_ctr < 32 ); // try 32 times maximum

	  // Section selection
	  // Approach:
	  // o enabled with t->play_section > 0
	  // o section width matches with the Track length, which means that the sequencer will
	  //   play steps beyond the "last step" with t->play_section > 0
	  // o section controlled via UI, MIDI Keyboard or BLM
	  // o lower priority than global loop mode
	  if( t->play_section > 0 ) {
	    // note: SEQ_TRG_Get() will return 0 if t->step beyond total track size - no need to consider this here
	    int step_offset = t->play_section * ((int)tcc->length+1);
	    t->step += step_offset;
	  }

	  // global loop mode handling
	  // requirements:
	  // o loop all or only select tracks
	  // o allow to loop the step view (16 step window) or a definable number of steps
	  // o wrap step position properly when loop mode is activated so that this
	  //   doesn't "dirsturb" the sequence output (ensure that the track doesn't get out of sync)
	  if( seq_core_state.LOOP ) {
	    u8 loop_active = 0;
	    int step_offset = 0;
	    
	    switch( seq_core_glb_loop_mode ) {
	    case SEQ_CORE_LOOP_MODE_ALL_TRACKS_STATIC:
	      loop_active = 1;
	      break;

	    case SEQ_CORE_LOOP_MODE_SELECTED_TRACK_STATIC:
	      if( SEQ_UI_IsSelectedTrack(track) )
		loop_active = 1;
	      break;

	    case SEQ_CORE_LOOP_MODE_ALL_TRACKS_VIEW:
	      loop_active = 1;
	      // no break!

	    case SEQ_CORE_LOOP_MODE_SELECTED_TRACK_VIEW:
	      if( SEQ_UI_IsSelectedTrack(track) )
		loop_active = 1;

	      step_offset = 16 * ui_selected_step_view;
	      break;
	    }

	    if( loop_active ) {
	      // wrap step position within given boundaries if required
	      step_offset += seq_core_glb_loop_offset;
	      step_offset %= ((int)tcc->length+1);

	      int loop_steps = seq_core_glb_loop_steps + 1;
	      int max_steps = (int)tcc->length + 1;
	      if( loop_steps > max_steps )
		loop_steps = max_steps;

	      int new_step = (int)t->step;
	      new_step = step_offset + ((new_step-step_offset) % loop_steps);

	      if( new_step > tcc->length )
		new_step = step_offset;
	      t->step = new_step;
	    }
	  }

	  // calculate number of cycles to next step
	  if( tcc->groove_style.sync_to_track ) {
	    t->timestamp_next_step = t->timestamp_next_step_ref + SEQ_GROOVE_DelayGet(track, t->step + 1);
	  } else {
	    t->timestamp_next_step = t->timestamp_next_step_ref + SEQ_GROOVE_DelayGet(track, seq_core_state.ref_step + 1);
	  }

	  if( !mute_this_step && !seq_record_options.FWD_MIDI && track_record_enabled ) { // if not already skipped (e.g. MANUAL mode)
	    mute_this_step = t->state.REC_DONT_OVERWRITE_NEXT_STEP;

	    if( seq_core_state.FIRST_CLK && seq_record_options.AUTO_START && seq_record_options.FWD_MIDI )
	      mute_this_step = 1; // mute initial step which is going to be recorded
	  }

	  // forward new step to recording function (only used in live recording mode)
	  if( track_record_enabled )
	    SEQ_RECORD_NewStep(track, prev_step, t->step, bpm_tick);

	  // forward to live function (for repeats)
	  // if it returns 1, the step won't be played
	  if( SEQ_LIVE_NewStep(track, prev_step, t->step, bpm_tick) == 1 )
	    mute_this_step = 1;

	  // inform UI about a new step (UI will clear this variable)
	  seq_core_step_update_req = 1;
	}

        // solo function: don't play MIDI event if track not selected
        // mute function
        // track disabled
	// mute for non-loopback tracks activated
	// MIDI player in exclusive mode
	// Record Mode, new step and FWD_MIDI off
	u8 track_soloed = seq_core_trk_soloed && (seq_core_trk_soloed & (1 << track));
        if( (!seq_core_trk_soloed && seq_ui_button_state.SOLO && !SEQ_UI_IsSelectedTrack(track)) ||
	    (seq_core_trk_soloed && !track_soloed) ||
	    (!track_soloed && (seq_core_trk_muted & (1 << track))) || // Track Mute function
	    seq_core_slaveclk_mute || // Slave Clock Mute Function
	    SEQ_MIDI_PORT_OutMuteGet(tcc->midi_port) || // Port Mute Function
	    tcc->playmode == SEQ_CORE_TRKMODE_Off || // track disabled
	    (round && mute_nonloopback_tracks) || // all non-loopback tracks should be muted
	    midply_solo || // MIDI player in exclusive mode
	    mute_this_step ) { // Record Mode, new step and FWD_MIDI off

	  if( t->state.STRETCHED_GL || t->state.SUSTAINED ) {
	    int i;

	    if( !t->state.STRETCHED_GL ) // important: play Note Off before new Note On to avoid that glide is triggered on the synth
	      SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick ? (bpm_tick-1) : 0,
				      track_record_enabled ? seq_record_played_notes : NULL);
	    else // Glide
	      SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick,
				      track_record_enabled ? seq_record_played_notes : NULL);

	    // clear state flags and note storage
	    t->state.STRETCHED_GL = 0;
	    t->state.SUSTAINED = 0;
	    for(i=0; i<4; ++i)
	      t->glide_notes[i] = 0;
	  }

	  continue;
	}

	// parameter layer mute flags (only if not in drum mode)
	u16 layer_muted = (tcc->event_mode != SEQ_EVENT_MODE_Drum) ? (t->layer_muted | t->layer_muted_from_midi) : 0;

        // if random gate trigger set: play step with 1:1 probability
        if( SEQ_TRG_RandomGateGet(track, t->step, 0) && (SEQ_RANDOM_Gen(0) & 1) )
	  continue;

	// check probability if not in drum mode
	// if probability < 100: play step with given probability
	// in drum mode, the probability is checked for each individual instrument inside the layer event loop
	if( tcc->event_mode != SEQ_EVENT_MODE_Drum ) {
	  u8 rnd_probability;
	  if( (rnd_probability=SEQ_PAR_ProbabilityGet(track, t->step, 0, layer_muted)) < 100 &&
	      SEQ_RANDOM_Gen_Range(0, 99) >= rnd_probability )
	    continue;
	}

	// store last glide notes before they will be cleared
	//memcpy(last_glide_notes, t->glide_notes, 4*4);
	// this will be a bit faster (memcpy copies bytes, by copying words we save some time)
	u32 prev_glide_notes[4];
	u32 next_glide_notes[4];
	{
	  u32 *src_ptr = (u32 *)&t->glide_notes[0];
	  u32 *dst_ptr = (u32 *)&prev_glide_notes[0];
	  u32 *next_ptr = (u32 *)&next_glide_notes[0];
	  int i;
	  for(i=0; i<4; ++i) {
	    *dst_ptr++ = *src_ptr++;
	    *next_ptr++ = 0;
	  }
	}
	
#ifdef MBSEQV4P
        seq_layer_evnt_t layer_events[83];
        s32 number_of_events = 0;
	number_of_events = SEQ_LAYER_GetEventsPlus(track, t->step, layer_events, 0);
#else
        seq_layer_evnt_t layer_events[16];
        s32 number_of_events = 0;
	number_of_events = SEQ_LAYER_GetEvents(track, t->step, layer_events, 0);
#endif

	if( number_of_events > 0 ) {
	  int i;

	  //////////////////////////////////////////////////////////////////////////////////////////
	  // First pass: handle length and apply functions which modify the MIDI events
	  //////////////////////////////////////////////////////////////////////////////////////////
	  u16 prev_bpm_tick_delay = t->bpm_tick_delay;
	  u8 gen_on_events = 0; // new On Events will be generated
	  u8 gen_sustained_events = 0; // new sustained On Events will be generated
	  u8 gen_off_events = 0; // Off Events of previous step will be generated, the variable contains the remaining gatelength

          seq_layer_evnt_t *e = &layer_events[0];
          for(i=0; i<number_of_events; ++e, ++i) {
            mios32_midi_package_t *p = &e->midi_package;

	    // instrument layers only used for drum tracks
	    u8 instrument = (tcc->event_mode == SEQ_EVENT_MODE_Drum) ? e->layer_tag : 0;

	    // individual for each instrument in drum mode:
	    // if probability < 100: play step with given probability
	    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
	      u8 rnd_probability;
	      if( (rnd_probability=SEQ_PAR_ProbabilityGet(track, t->step, instrument, layer_muted)) < 100 &&
		  SEQ_RANDOM_Gen_Range(0, 99) >= rnd_probability )
		continue;
	    }

	    // get nofx flag
	    robotize_flags = SEQ_ROBOTIZE_Event(track, t->step, e);
	    u8 no_fx = SEQ_TRG_NoFxGet(track, t->step, instrument);

	    // get nth trigger flag
	    // note: this check will be done again during the second pass for some triggers which are not handled during first pass
	    u8 nth_trigger = 0;
	    {
	      u8 nth_variant = 0; // Nth1 or Nth2
	      u8 nth_value = SEQ_PAR_Nth1ValueGet(track, t->step, instrument, layer_muted);
	      if( !nth_value ) {
		nth_variant = 1;
		nth_value = SEQ_PAR_Nth2ValueGet(track, t->step, instrument, layer_muted);
	      }

	      if( nth_value ) {
		int bar = nth_value & 0xf;
		int trigger = nth_variant ? ((t->bar % (bar+1)) == bar) : ((t->bar % (bar+1)) == 0);

		int mode = (nth_value >> 4) & 0x7;
		if( mode == SEQ_PAR_TYPE_NTH_PLAY ) {
		  if( !trigger )
		    continue; // step not played
		} else if( mode == SEQ_PAR_TYPE_NTH_MUTE ) {
		  if( trigger )
		    continue; // step not played
		} else if( mode == SEQ_PAR_TYPE_NTH_FX ) {
		  if( !trigger )
		    no_fx = 1;
		} else if( mode == SEQ_PAR_TYPE_NTH_NO_FX ) {		  
		  if( trigger )
		    no_fx = 1;
		} else {
		  if( trigger )
		    nth_trigger = mode;
		}
	      }
	    }

            // transpose notes/CCs
            SEQ_CORE_Transpose(track, instrument, t, tcc, p);

            // glide trigger
            if( e->len > 0 && tcc->event_mode != SEQ_EVENT_MODE_Drum ) {
	      if( SEQ_TRG_GlideGet(track, t->step, instrument) )
		e->len = 96; // Glide
            }

	    // if glided note already played: omit new note event by setting velocity to 0
	    if( t->state.STRETCHED_GL ) {
	      u32 ix = p->note / 32;
	      u32 mask = (1 << (p->note % 32));
	      if( prev_glide_notes[ix] & mask ) {
		if( e->len >= 96 ) {
		  next_glide_notes[ix] |= mask;
		}
		p->velocity = 0;
	      }
	    }
  
            // skip if velocity has been cleared by transpose or glide function
            // (e.g. no key pressed in transpose mode)
            if( p->type == NoteOn && !p->velocity ) {
	      // stretched note, length < 96: queue off event
	      if( t->state.STRETCHED_GL && t->state.SUSTAINED && (e->len < 96) )
		gen_off_events = (t->step_length * e->len) / 96;
	      continue;
	    }

	    // get delay of step (0..95)
	    // note: negative delays stored in step parameters would require to pre-generate bpm_ticks, 
	    // which would reduce the immediate response on value/trigger changes
	    // therefore negative delays are only supported for groove patterns, and they are
	    // applied over the whole track (e.g. drum mode: all instruments of the appr. track)
	    t->bpm_tick_delay = SEQ_PAR_StepDelayGet(track, t->step, instrument, layer_muted);

	    // scale delay (0..95) over next clock counter to consider the selected clock divider
	    if( t->bpm_tick_delay )
	      t->bpm_tick_delay = (t->bpm_tick_delay * t->step_length) / 96;


            if( p->type != NoteOn ) {
	      // apply Pre-FX
	      if( !no_fx ) {
		SEQ_LFO_Event(track, e);
	      }

            } else if( p->note && p->velocity && (e->len >= 0) ) {
	      // Note Event

	      // groove it
	      if( tcc->groove_style.sync_to_track ) {
		SEQ_GROOVE_Event(track, t->step, e);
	      } else {
		SEQ_GROOVE_Event(track, seq_core_state.ref_step, e);
	      }

	      // apply Pre-FX before force-to-scale
	      if( !no_fx ) {
		SEQ_HUMANIZE_Event(track, t->step, e);
			
		if( !robotize_flags.NOFX ) {
		  SEQ_LFO_Event(track, e);
		}
	      }

	      t->state.ROBOSUSTAINED = ( robotize_flags.SUSTAIN ) ? 1 : 0 ;// set robosustain flag

	      // force to scale
	      if( tcc->trkmode_flags.FORCE_SCALE ) {
		u8 scale, root_selection, root;
		SEQ_CORE_FTS_GetScaleAndRoot(track, t->step, instrument, tcc, &scale, &root_selection, &root);
		SEQ_SCALE_Note(p, scale, root);
	      }

	      // apply Pre-FX after force-to-scale
	      if( !no_fx ) {
		SEQ_CORE_Limit(t, tcc, e); // should be the last Fx in the chain!
	      }

	      // force velocity to 0x7f (drum mode: selectable value) if accent flag set
	      if( nth_trigger == SEQ_PAR_TYPE_NTH_ACCENT || SEQ_TRG_AccentGet(track, t->step, instrument) ) {
		if( tcc->event_mode == SEQ_EVENT_MODE_Drum )
		  p->velocity = tcc->lay_const[2*16 + i];
		else
		  p->velocity = 0x7f;
	      }

	      // sustained or stretched note: play off event of previous step
	      if( t->state.SUSTAINED )
		gen_off_events = 1;

	      if( tcc->trkmode_flags.SUSTAIN || t->state.ROBOSUSTAINED || e->len >= 96 )
		gen_sustained_events = 1;
	      else {
		// generate common On event with given length
		gen_on_events = 1;
	      }
	    } else if( t->state.STRETCHED_GL && t->state.SUSTAINED && (e->len < 96) ) {
	      // stretched note, length < 96: queue off events
	      gen_off_events = (t->step_length * e->len) / 96;
	    }
	  }

	  // should Note Off events be played before new events are queued?
	  if( gen_off_events ) {
	    u32 rescheduled_tick = bpm_tick + prev_bpm_tick_delay + gen_off_events;
	    if( !t->state.STRETCHED_GL ) // important: play Note Off before new Note On to avoid that glide is triggered on the synth
	      rescheduled_tick -= 1;

	    SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, rescheduled_tick,
				    track_record_enabled ? seq_record_played_notes : (t->state.STRETCHED_GL ? next_glide_notes : NULL));

	    // clear state flag and note storage
	    t->state.SUSTAINED = 0;
	    if( track_record_enabled || !t->state.STRETCHED_GL ) {
	      t->state.STRETCHED_GL = 0;

	      u32 *dst_ptr = (u32 *)&t->glide_notes[0];
	      int i;
	      for(i=0; i<4; ++i) {
		*dst_ptr++ = 0;
	      }
	    } else if( t->state.STRETCHED_GL ) {
	      // improved glide handling for polyphonic steps
	      u8 any_glide = 0;
	      u32 *src_ptr = (u32 *)&next_glide_notes[0];
	      u32 *dst_ptr = (u32 *)&t->glide_notes[0];
	      int i;
	      for(i=0; i<4; ++i) {
		if( *src_ptr )
		  any_glide |= 1;

		*dst_ptr++ = *src_ptr++;
	      }

	      if( !any_glide )
		t->state.STRETCHED_GL = 0;
	    }
	  }


	  //////////////////////////////////////////////////////////////////////////////////////////
	  // Second pass: schedule new events
	  //////////////////////////////////////////////////////////////////////////////////////////
          e = &layer_events[0];
	  u8 reset_stacks_done = 0;
          for(i=0; i<number_of_events; ++e, ++i) {
            mios32_midi_package_t *p = &e->midi_package;

	    // instrument layers only used for drum tracks
	    u8 instrument = (tcc->event_mode == SEQ_EVENT_MODE_Drum) ? e->layer_tag : 0;

	    robotize_flags = SEQ_ROBOTIZE_Event(track, t->step, e);
	    u8 no_fx = SEQ_TRG_NoFxGet(track, t->step, instrument);

	    // get nth trigger flag
	    // note: this check was already done during first pass, do it here again for triggers which are handled in the second pass
	    u8 nth_trigger = 0;
	    {
	      u8 nth_variant = 0; // Nth1 or Nth2
	      u8 nth_value = SEQ_PAR_Nth1ValueGet(track, t->step, instrument, layer_muted);
	      if( !nth_value ) {
		nth_variant = 1;
		nth_value = SEQ_PAR_Nth2ValueGet(track, t->step, instrument, layer_muted);
	      }

	      if( nth_value ) {
		int bar = nth_value & 0xf;
		int trigger = nth_variant ? ((t->bar % (bar+1)) == bar) : ((t->bar % (bar+1)) == 0);

		int mode = (nth_value >> 4) & 0x7;
		if( mode == SEQ_PAR_TYPE_NTH_PLAY ) {
		  if( !trigger )
		    continue; // step not played
		} else if( mode == SEQ_PAR_TYPE_NTH_MUTE ) {
		  if( trigger )
		    continue; // step not played
		} else if( mode == SEQ_PAR_TYPE_NTH_FX ) {
		  if( !trigger )
		    no_fx = 1;
		} else if( mode == SEQ_PAR_TYPE_NTH_NO_FX ) {		  
		  if( trigger )
		    no_fx = 1;
		} else {
		  if( trigger )
		    nth_trigger = mode;
		}
	      }
	    }

	    if( p->type != NoteOn ) {
	      // e.g. CC, PitchBend, ProgramChange
	      if( loopback_port )
		SEQ_MIDI_IN_BusReceive(tcc->midi_port & 0x0f, *p, 1); // forward to MIDI IN handler immediately
	      else
		SEQ_CORE_ScheduleEvent(track, t, tcc, *p, SEQ_MIDI_OUT_CCEvent, bpm_tick + t->bpm_tick_delay, 0, 0, robotize_flags);
	      t->vu_meter = 0x7f; // for visualisation in mute menu
	    } else {
	      // skip in record mode if the same note is already played
	      if( track_record_enabled && t->state.STRETCHED_GL &&
		  (seq_record_played_notes[p->note>>5] & (1 << (p->note&0x1f))) )
		continue;

	      // skip in glide mode if note stretched
	      if( t->state.STRETCHED_GL &&
		  (next_glide_notes[p->note>>5] & (1 << (p->note&0x1f))) )
		continue;

	      // sustained/glided note: play note at timestamp, and queue off event at 0xffffffff (so that it can be re-scheduled)		
	      if( gen_sustained_events ) {
		// for visualisation in mute menu
		t->vu_meter = p->velocity;

		if( loopback_port ) {
		  if( !reset_stacks_done ) {
		    // reset current stack
		    SEQ_MIDI_IN_ResetSingleTransArpStacks(tcc->midi_port & 0x0f);
		    reset_stacks_done = 1;
		  }
		  // forward to MIDI IN handler immediately
		  SEQ_MIDI_IN_BusReceive(tcc->midi_port & 0x0f, *p, 1);
		} else {
		  u32 scheduled_tick = bpm_tick + t->bpm_tick_delay;

		  // glide: if same note already played, play the new one a tick later for 
		  // proper handling of "fingered portamento" function on some synths
		  if( prev_glide_notes[p->note / 32] & (1 << (p->note % 32)) )
		    scheduled_tick += 1;

		  // Note On (the Note Off will be prepared as well in SEQ_CORE_ScheduleEvent)
		  SEQ_CORE_ScheduleEvent(track, t, tcc, *p, SEQ_MIDI_OUT_OnEvent, scheduled_tick, 0, 0, robotize_flags);

		  // apply Post-FX
		  if( !no_fx && !robotize_flags.NOFX ) {
		    u8 local_gatelength = 95; // echo only with reduced gatelength to avoid killed notes

		    SEQ_CORE_Echo(track, instrument, t, tcc, *p, bpm_tick + t->bpm_tick_delay, local_gatelength, robotize_flags);
		  }
		}

		// notify stretched gatelength if not in sustain mode
		t->state.SUSTAINED = 1;
		if( !tcc->trkmode_flags.SUSTAIN && !t->state.ROBOSUSTAINED ) {
		  t->state.STRETCHED_GL = 1;
		  // store glide note number in 128 bit array for later checks
		  t->glide_notes[p->note / 32] |= (1 << (p->note % 32));
		}

	      } else if( gen_on_events ) {
		// for visualisation in mute menu
		t->vu_meter = p->velocity;

		if( loopback_port ) {
		  if( !reset_stacks_done ) {
		    reset_stacks_done = 1;
		    // reset current stack
		    SEQ_MIDI_IN_ResetSingleTransArpStacks(tcc->midi_port & 0x0f);
		  }
		  // forward to MIDI IN handler immediately
		  SEQ_MIDI_IN_BusReceive(tcc->midi_port & 0x0f, *p, 1);
		  // multi triggers, but also echo not possible on loopback ports
		} else {
		  u16 gatelength = e->len;
		  u8 triggers = 1;

		  // roll/flam?
		  // get roll mode from parameter layer
		  u8 roll_mode = 0;
		  u8 roll2_mode = 0; // taken if roll1 not assigned
		  if( SEQ_TRG_RollGateGet(track, t->step, instrument) ) { // optional roll gate
		    roll_mode = SEQ_PAR_RollModeGet(track, t->step, instrument, layer_muted);
		    // with less priority (parameter == 0): force roll mode if Roll trigger is set
		    if( nth_trigger == SEQ_PAR_TYPE_NTH_ROLL || (!roll_mode && SEQ_TRG_RollGet(track, t->step, instrument)) )
		      roll_mode = 0x0a; // 2D10
		    // if roll mode != 0: increase number of triggers
		    if( roll_mode ) {
		      triggers = ((roll_mode & 0x30)>>4) + 2;
		    } else {
		      roll2_mode = SEQ_PAR_Roll2ModeGet(track, t->step, instrument, layer_muted);
		      if( roll2_mode )
			triggers = (roll2_mode >> 5) + 2;
		    }
		  }

		  if( triggers > 1 ) {
		    if( roll2_mode ) {
		      // force gatelength depending on roll2 value
		      gatelength = (8 - 2*(roll2_mode >> 5)) * ((roll2_mode&0x1f)+1);

		      // scale length (0..95) over next clock counter to consider the selected clock divider
		      int gatelength = (4 - (roll2_mode >> 5)) * ((roll2_mode&0x1f)+1);

		      u32 half_gatelength = gatelength/2;
		      if( !half_gatelength )
			half_gatelength = 1;
      	      
		      int i;
		      for(i=triggers-1; i>=0; --i)
			SEQ_CORE_ScheduleEvent(track, t, tcc, *p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay + i*gatelength, half_gatelength, 0, robotize_flags);
		    } else {
		      // force gatelength depending on number of triggers
		      if( triggers < 6 ) {
			//       number of triggers:    2   3   4   5
			const u8 gatelength_tab[4] = { 48, 32, 36, 32 };
			// strategy:
			// 2 triggers: played within 1 step at 0 and 48
			// 3 triggers: played within 1 step at 0, 32 and 64
			// 4 triggers: played within 1.5 steps at 0, 36, 72 and 108
			// 5 triggers: played within 1.5 steps at 0, 32, 64, 96 and 128

			// in addition, scale length (0..95) over next clock counter to consider the selected clock divider
			gatelength = (gatelength_tab[triggers-2] * t->step_length) / 96;
		      }

		      u32 half_gatelength = gatelength/2;
		      if( !half_gatelength )
			half_gatelength = 1;
      	      
		      mios32_midi_package_t p_multi = *p;
		      u16 roll_attenuation = 256 - (2 * triggers * (16 - (roll_mode & 0x0f))); // magic formula for nice effects
		      if( roll_mode & 0x40 ) { // upwards
			int i;
			for(i=triggers-1; i>=0; --i) {
			  SEQ_CORE_ScheduleEvent(track, t, tcc, p_multi, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay + i*gatelength, half_gatelength ,0, robotize_flags);
			  u16 velocity = roll_attenuation * p_multi.velocity;
			  p_multi.velocity = velocity >> 8;
			}
		      } else { // downwards
			int i;
			for(i=0; i<triggers; ++i) {
			  SEQ_CORE_ScheduleEvent(track, t, tcc, p_multi, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay + i*gatelength, half_gatelength, 0, robotize_flags);
			  if( roll_mode ) {
			    u16 velocity = roll_attenuation * p_multi.velocity;
			    p_multi.velocity = velocity >> 8;
			  }
			}
		      }
		    }
		  } else {
		    if( !gatelength )
		      gatelength = 1;
		    else // scale length (0..95) over next clock counter to consider the selected clock divider
		      gatelength = (gatelength * t->step_length) / 96;
		    SEQ_CORE_ScheduleEvent(track, t, tcc, *p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay, gatelength, 0, robotize_flags);
		  }

		  // apply Post-FX
		  if( !no_fx && !robotize_flags.NOFX) {
		    if( ( (tcc->echo_repeats & 0x3f) && ( !(tcc->echo_repeats & 0x40) || robotize_flags.ECHO ) && gatelength ) )
		      SEQ_CORE_Echo(track, instrument, t, tcc, *p, bpm_tick + t->bpm_tick_delay, gatelength, robotize_flags);
		  }
		}
	      }
            }
          }
        }
      }
    }
  }

  // clear "first clock" flag if it was set before
  seq_core_state.FIRST_CLK = 0;

  // if manual trigger function requested stop: stop sequencer at end of reference step
  if( seq_core_state.MANUAL_TRIGGER_STOP_REQ && (bpm_tick % 96) == 95 )
    SEQ_BPM_Stop();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets the step position variables of a track
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_ResetTrkPos(u8 track, seq_core_trk_t *t, seq_cc_trk_t *tcc)
{
  // synch to measure done
  t->state.SYNC_MEASURE = 0;

  // don't increment on first clock event
  t->state.FIRST_CLK = 1;

  // clear delay
  t->bpm_tick_delay = 0;

  // reset step progression counters
  t->step_replay_ctr = 0;
  t->step_fwd_ctr = 0;
  t->step_interval_ctr = 0;
  t->step_repeat_ctr = 0;
  t->step_skip_ctr = 0;

  // and MIDI Fx channel counter
  t->fx_midi_ctr = 0xff; // start with original channel

  // next part depends on forward/backward direction
  if( tcc->dir_mode == SEQ_CORE_TRKDIR_Backward ) {
    // only for Backward mode
    t->state.BACKWARD = 1;
    t->step = tcc->length;
  } else {
    // for Forward/PingPong/Pendulum/Random/...
    t->state.BACKWARD = 0;
    t->step = 0;
  }

  // save position (for repeat function)
  t->step_saved = t->step;

  t->arp_pos = 0;

  // reset LFO
  SEQ_LFO_ResetTrk(track);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets the step position variables of all tracks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_ResetTrkPosAll(void)
{
  seq_core_trk_t *t = &seq_core_trk[0];
  seq_cc_trk_t *tcc = &seq_cc_trk[0];
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t, ++tcc)
    SEQ_CORE_ResetTrkPos(track, t, tcc);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sets the track position for the given track (optionally scaled over 7bit)
// the manual flag will set the step immediately, and play it
/////////////////////////////////////////////////////////////////////////////
extern s32 SEQ_CORE_SetTrkPos(u8 track, u8 value, u8 scale_value)
{
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // invalid track

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  // scale CC value over track length
  // selectable steps: 128 maximum
  int step = value;
  if( scale_value ) {
    int length = (int)tcc->length + 1;
    if( length > 128 )
      length = 128;
    step = ((step * length) / 128);
  }

  if( !SEQ_BPM_IsRunning() ) {
    // reset track position
    SEQ_CORE_ResetTrkPos(track, t, tcc);

    // change step
    t->step = step;
    t->step_saved = t->step;
  } else {
    // request next step
    t->manual_step = step;
    t->state.MANUAL_STEP_REQ = 1;

    if( tcc->clkdiv.MANUAL )
      t->state.FIRST_CLK = 1; // change to step immediately (not synchronized)
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Determine next step depending on direction mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, u8 no_progression, u8 reverse)
{
  int i;
  u8 save_step = 0;
  u8 new_step = 1;

  // handle progression parameters if position shouldn't be reset
  if( !no_progression && !t->state.POS_RESET ) {
    if( ++t->step_fwd_ctr > tcc->steps_forward ) {
      t->step_fwd_ctr = 0;
      if( tcc->steps_jump_back ) {
	for(i=0; i<tcc->steps_jump_back; ++i)
	  SEQ_CORE_NextStep(t, tcc, 1, 1); // 1=no progression, 1=reverse
      }
      if( ++t->step_replay_ctr > tcc->steps_replay ) {
	t->step_replay_ctr = 0;
	save_step = 1; // request to save the step in t->step_saved at the end of this routine
      } else {
	t->step = t->step_saved;
	new_step = 0; // don't calculate new step
      }
    }

    if( ++t->step_interval_ctr > tcc->steps_rs_interval ) {
      t->step_interval_ctr = 0;
      t->step_repeat_ctr = tcc->steps_repeat;
      t->step_skip_ctr = tcc->steps_skip;
    }

    if( t->step_repeat_ctr ) {
      --t->step_repeat_ctr;
      new_step = 0; // repeat step until counter reached zero
    } else {
      while( t->step_skip_ctr ) {
	SEQ_CORE_NextStep(t, tcc, 1, 0); // 1=no progression, 0=not reverse
	--t->step_skip_ctr;
      }
    }
  }

  if( new_step ) {
    // special cases:
    switch( tcc->dir_mode ) {
      case SEQ_CORE_TRKDIR_Forward:
	t->state.BACKWARD = 0; // force backward flag
	break;

      case SEQ_CORE_TRKDIR_Backward:
	t->state.BACKWARD = 1; // force backward flag
	break;

      case SEQ_CORE_TRKDIR_PingPong:
      case SEQ_CORE_TRKDIR_Pendulum:
	// nothing else to do...
	break;

      case SEQ_CORE_TRKDIR_Random_Dir:
	// set forward/backward direction with 1:1 probability
	t->state.BACKWARD = SEQ_RANDOM_Gen(0) & 1;
        break;

      case SEQ_CORE_TRKDIR_Random_Step:
	t->step = SEQ_RANDOM_Gen_Range(tcc->loop, tcc->length);
	new_step = 0; // no new step calculation required anymore
        break;

      case SEQ_CORE_TRKDIR_Random_D_S:
	{
	  // we continue with a probability of 50%
	  // we change the direction with a probability of 25%
	  // we jump to a new step with a probability of 25%
	  u32 rnd;
	  if( ((rnd=SEQ_RANDOM_Gen(0)) & 0xff) < 0x80 ) {
	    if( rnd < 0x40 ) {
	      // set forward/backward direction with 1:1 probability
	      t->state.BACKWARD = SEQ_RANDOM_Gen(0) & 1;
	    } else {
	      t->step = SEQ_RANDOM_Gen_Range(tcc->loop, tcc->length);
	      new_step = 0; // no new step calculation required anymore
	    }
	  }
	}
	break;
    }
  }

  if( new_step ) { // note: new_step will be cleared in SEQ_CORE_TRKDIR_Random_Step mode
    // branch depending on forward/backward mode, take reverse flag into account
    if( t->state.BACKWARD ^ reverse ) {
      // jump to last step if first loop step has been reached or a position reset has been requested
      // in pendulum mode: switch to forward direction
      if( t->state.POS_RESET || t->step <= tcc->loop ) {
	++t->bar;

	if( tcc->dir_mode == SEQ_CORE_TRKDIR_Pendulum ) {
	  t->state.BACKWARD = 0;
	} else {
	  t->step = tcc->length;
	}
	// reset arp position as well
	t->arp_pos = 0;
      } else {
	// no reset required; decrement step
	--t->step;

	// in pingpong mode: turn direction if loop step has been reached after this decrement
	if( t->step <= tcc->loop && tcc->dir_mode == SEQ_CORE_TRKDIR_PingPong )
	  t->state.BACKWARD = 0;
      }
    } else {
      // jump to first (loop) step if last step has been reached or a position reset has been requested
      // in pendulum mode: switch to backward direction
      if( t->state.POS_RESET || t->step >= tcc->length ) {
	++t->bar;

	if( tcc->dir_mode == SEQ_CORE_TRKDIR_Pendulum ) {
	  t->state.BACKWARD = 1;
	} else {
	  t->step = tcc->loop;
	}
	// reset arp position as well
	t->arp_pos = 0;
      } else {
	// no reset required; increment step
	++t->step;

	// in pingpong mode: turn direction if last step has been reached after this increment
	if( t->step >= tcc->length && tcc->dir_mode == SEQ_CORE_TRKDIR_PingPong )
	  t->state.BACKWARD = 1;
      }
    }
  }

  if( !reverse ) {
    // requested by progression handler
    if( save_step )
      t->step_saved = t->step;

    t->state.POS_RESET = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Transposes if midi_package contains a Note Event
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Transpose(u8 track, u8 instrument, seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *p)
{
  u8 is_cc = p->type != NoteOn && p->type != NoteOff; // CC, Pitchbender, Programchange

  if( is_cc && tcc->event_mode != SEQ_EVENT_MODE_CC ) // only transpose CC/Pitchbender/Program Change in CC mode
    return -1;

  int note = is_cc ? p->value : p->note;

  int inc_oct = tcc->transpose_oct;
  if( inc_oct >= 8 )
    inc_oct -= 16;

  int inc_semi = tcc->transpose_semi;
  if( inc_semi >= 8 )
    inc_semi -= 16;

  // in transpose or arp playmode we allow to transpose notes and CCs
  if( tcc->playmode == SEQ_CORE_TRKMODE_Transpose ||
      (!is_cc && seq_core_global_transpose_enabled) ) {
    int tr_note = SEQ_MIDI_IN_TransposerNoteGet(tcc->busasg.bus, tcc->trkmode_flags.HOLD, tcc->trkmode_flags.FIRST_NOTE);

    if( tr_note < 0 ) {
      p->velocity = 0; // disable note and exit
      return -1; // note has been disabled
    }

    inc_semi += tr_note - 0x3c; // C-3 is the base note
  } else if( tcc->playmode == SEQ_CORE_TRKMODE_Arpeggiator ) {
    int key_num = (note >> 2) & 0x3;
    int arp_oct = (note >> 4) & 0x7;

    if( arp_oct < 2 ) { // Multi Arp Event
      inc_oct += ((note >> 2) & 7) - 4;
      key_num = t->arp_pos;
    } else {
      inc_oct += arp_oct - 4;
      t->arp_pos = 0; // ensure that no multi arp event is played anymore
    }

    int arp_note = SEQ_MIDI_IN_ArpNoteGet(tcc->busasg.bus, tcc->trkmode_flags.HOLD, !tcc->trkmode_flags.UNSORTED, key_num);

    if( arp_note & 0x80 ) {
      t->arp_pos = 0;
    } else {
      if( arp_oct < 2 ) { // Multi Arp Event
	// play next key, step will be incremented once t->arp_pos reaches 0 again
	if( ++t->arp_pos >= 4 )
	  t->arp_pos = 0;
      }
    }

    note = arp_note & 0x7f;

    if( !note ) { // disable note and exit
      p->velocity = 0;
      return -1; // note has been disabled
    }
  } else {
    // neither transpose nor arpeggiator mode: transpose based on root note if specified in parameter layer
    if( !is_cc && tcc->link_par_layer_root >= 0 ) {
      u8 root = SEQ_PAR_Get(track, t->step, tcc->link_par_layer_root, instrument);
      if( !root ) {
	root = seq_core_global_scale_root_selection;
      }

      if( root ) {
	inc_semi += root - 1;
      } else {
	int tr_note = SEQ_MIDI_IN_TransposerNoteGet(tcc->busasg.bus, tcc->trkmode_flags.HOLD, tcc->trkmode_flags.FIRST_NOTE);

	if( tr_note < 0 ) {
	  p->velocity = 0; // disable note and exit
	  return -1; // note has been disabled
	}

	inc_semi += tr_note - 0x3c; // C-3 is the base note
      }
    }
  }

  // apply transpose octave/semitones parameter
  if( inc_oct ) {
    note += 12 * inc_oct;
  }

  if( inc_semi ) {
    note += inc_semi;
  }

  // ensure that note is in the 0..127 range
  note = SEQ_CORE_TrimNote(note, 0, 127);

  if( is_cc ) // if CC, Pitchbender, ProgramChange
    p->value = note;
  else
    p->note = note;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the selected scale and root note selection depending on
// global/group specific settings or local track if the appr. parameter 
// layers are available
// scale and root note are for interest while playing the sequence -> SEQ_CORE
// scale and root selection are for interest when editing the settings -> SEQ_UI_OPT
// Both modules are calling this function to ensure consistency
// 
// if *tcc is NULL, the function will always return the global settings
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_FTS_GetScaleAndRoot(u8 track, u8 step, u8 instrument, seq_cc_trk_t *tcc, u8 *scale, u8 *root_selection, u8 *root)
{
  if( tcc && tcc->link_par_layer_scale >= 0 ) {
    *scale = SEQ_PAR_Get(track, step, tcc->link_par_layer_scale, instrument);
    if( *scale ) {
      *scale -= 1;
    } else {
      *scale = seq_core_global_scale;
    }
  } else {
    *scale = seq_core_global_scale;
  }

  *root_selection = seq_core_global_scale_root_selection;
  if( tcc && tcc->link_par_layer_root >= 0 ) {
    *root = SEQ_PAR_Get(track, step, tcc->link_par_layer_root, instrument);
    if( *root ) {
      *root -= 1;
    } else {
      *root = (*root_selection == 0) ? seq_core_keyb_scale_root : (*root_selection-1);
    }
  } else {
    *root = (*root_selection == 0) ? seq_core_keyb_scale_root : (*root_selection-1);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Limit Fx
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Limit(seq_core_trk_t *t, seq_cc_trk_t *tcc, seq_layer_evnt_t *e)
{
  u8 lower = tcc->limit_lower;
  u8 upper = tcc->limit_upper;

  // check if any limit defined
  if( !lower && !upper )
    return 0; // no limit

  // exit if no note event
  mios32_midi_package_t *p = &e->midi_package;
  if( p->type != NoteOn )
    return 0; // no Note

  // if not set: allow full range
  if( !upper )
    upper = 127;

  // swap if lower value is greater than upper
  if( lower > upper ) {
    u8 tmp = upper;
    upper=lower;
    lower=tmp;
  }

  // apply limit
  p->note = SEQ_CORE_TrimNote(p->note, lower, upper);

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Name of Delay mode (we should outsource the echo function to seq_echo.c later)
/////////////////////////////////////////////////////////////////////////////
// Note: newer gcc versions don't allow to return a "const" parameter, therefore
// this array is declared outside the SEQ_CORE_Echo_GetDelayModeName() function

#define NUM_DELAY_VALUES 23
static const char delay_str[NUM_DELAY_VALUES+1][5] = {
    " 64T",
    " 64 ",
    " 32T",
    " 32 ",
    " 16T",
    " 16 ",
    "  8T",
    "  8 ",
    "  4T",
    "  4 ",
    "  2T",
    "  2 ",
    "  1T",
    "  1 ",
    "Rnd1",
    "Rnd2",
    " 64d", // new with Beta30
    " 32d", // new with Beta30
    " 16d", // new with Beta30
    "  8d", // new with Beta30
    "  4d", // new with Beta30
    "  2d", // new with Beta30
    "  0 ", // new with Beta42
    "????",
  };

const char *SEQ_CORE_Echo_GetDelayModeName(u8 delay_mode)
{
  if( delay_mode < NUM_DELAY_VALUES )
    return delay_str[delay_mode];

  return delay_str[NUM_DELAY_VALUES];
}


/////////////////////////////////////////////////////////////////////////////
// Maps delay values from old to new format
// Used to keep pattern binaries compatible to enhanced delay entries
/////////////////////////////////////////////////////////////////////////////
static const u8 delay_value_map[NUM_DELAY_VALUES] = {
  22, //"  0 ",
  0,  //" 64T",
  1,  //" 64 ",
  2,  //" 32T",
  16, //" 64d",
  3,  //" 32 ",
  4,  //" 16T",
  17, //" 32d",
  5,  //" 16 ",
  6,  //"  8T",
  18, //" 16d",
  7,  //"  8 ",
  8,  //"  4T",
  19, //"  8d",
  9,  //"  4 ",
  10, //"  2T",
  20, //"  4d",
  11, //"  2 ",
  12, //"  1T",
  21, //"  2d",
  13, //"  1 ",
  14, //"Rnd1",
  15, //"Rnd2",
};

u8 SEQ_CORE_Echo_MapUserToInternal(u8 user_value)
{
  if( user_value < NUM_DELAY_VALUES )
    return delay_value_map[user_value];

  return 0;
}

u8 SEQ_CORE_Echo_MapInternalToUser(u8 internal_value)
{
  int i;
  for(i=0; i<NUM_DELAY_VALUES; ++i)
    if( delay_value_map[i] == internal_value )
      return i;

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Echo Fx
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Echo(u8 track, u8 instrument, seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t p, u32 bpm_tick, u32 gatelength, seq_robotize_flags_t robotize_flags)
{
  // thanks to MIDI queuing mechanism, this is a no-brainer :)

  // 64T, 64, 32T, 32, 16T, 16, ... 1, Rnd1 and Rnd2, 64d..2d (new), 0 (supernew)
  s32 fb_ticks;
  s32 echo_delay = tcc->echo_delay;
  if( echo_delay >= 22 ) // new zero delay
    fb_ticks = 0;
  else if ( echo_delay >= 16 ) // new dotted delays
    fb_ticks = 36 * (1 << (echo_delay-16));
  else {
    if( echo_delay >= 14 ) // Rnd1 and Rnd2
      echo_delay = SEQ_RANDOM_Gen_Range(3, 7); // between 32 and 8
    fb_ticks = ((tcc->echo_delay & 1) ? 24 : 16) * (1 << (echo_delay>>1));
  }

  s32 fb_note = p.note;
  s32 fb_note_base = fb_note; // for random function

  // the initial velocity value allows to start with a low velocity,
  // and to increase it with each step via FB velocity value
  s32 fb_velocity = p.velocity;
  if( tcc->echo_velocity != 20 ) { // 20 == 100% -> no change
    fb_velocity = (fb_velocity * 5*tcc->echo_velocity) / 100;
    if( fb_velocity > 127 )
      fb_velocity = 127;
    p.velocity = (u8)fb_velocity;
  }

  seq_midi_out_event_type_t event_type = SEQ_MIDI_OUT_OnOffEvent;
  if( (p.type == CC || p.type == PitchBend || p.type == ProgramChange) && !gatelength )
    event_type = SEQ_MIDI_OUT_CCEvent;

  // for the case that force-to-scale is activated
  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(track, t->step, instrument, tcc, &scale, &root_selection, &root);

  u32 echo_offset = fb_ticks;
  u8 echo_repeats = tcc->echo_repeats;

  if( robotize_flags.ECHO ) {
	// remove 0x40 flag indicating that echo is active (it's reversed, so 1 indicates echo is set to off)
	// have to strip this flag out or the MSB flag makes a huge # of echo_repeats.
	echo_repeats = echo_repeats & 0x0F;
  }
	
  if( echo_repeats & 0x40 && !robotize_flags.ECHO) // disable flag
    echo_repeats = 0;
    
    
  int i;
  for(i=0; i<echo_repeats; ++i) {
    if( i ) { // no feedback of velocity or echo ticks on first step
      if( tcc->echo_fb_velocity != 20 ) { // 20 == 100% -> no change
	fb_velocity = (fb_velocity * 5*tcc->echo_fb_velocity) / 100;
	if( fb_velocity > 127 )
	  fb_velocity = 127;
	p.velocity = (u8)fb_velocity;
      }

      if( tcc->echo_fb_ticks != 20 ) { // 20 == 100% -> no change
	fb_ticks = (fb_ticks * 5*tcc->echo_fb_ticks) / 100;
      }
      echo_offset += fb_ticks;
    }

    if( tcc->echo_fb_note != 24 ) { // 24 == 0 -> no change
      if( tcc->echo_fb_note == 49 ) // random
	fb_note = fb_note_base + ((s32)SEQ_RANDOM_Gen_Range(0, 48) - 24);
      else
	fb_note = fb_note + ((s32)tcc->echo_fb_note-24);

      // ensure that note is in the 0..127 range
      fb_note = SEQ_CORE_TrimNote(fb_note, 0, 127);

      p.note = (u8)fb_note;
    }

    if( gatelength && tcc->echo_fb_gatelength != 20 ) { // 20 == 100% -> no change
      gatelength = (gatelength * 5*tcc->echo_fb_gatelength) / 100;
      if( !gatelength )
	gatelength = 1;
    }

    // force to scale
    if( tcc->trkmode_flags.FORCE_SCALE ) {
      SEQ_SCALE_Note(&p, scale, root);
    }

    SEQ_CORE_ScheduleEvent(track, t, tcc, p, event_type, bpm_tick + echo_offset, gatelength, 1, robotize_flags);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Manually triggers a step of all selected tracks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_ManualTrigger(u8 step)
{
  MIOS32_IRQ_Disable();

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    if( SEQ_UI_IsSelectedTrack(track) ) {
      SEQ_CORE_SetTrkPos(track, step, 0);
    }
  }

  if( !SEQ_BPM_IsRunning() ) {
    // start sequencer if not running, but only for one step
    SEQ_BPM_Cont();
    seq_core_state.MANUAL_TRIGGER_STOP_REQ = 1;
  }

  MIOS32_IRQ_Enable();

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Manually requests synch to measure for given tracks
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_ManualSynchToMeasure(u16 tracks)
{
  MIOS32_IRQ_Disable();

  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t)
    if( tracks & (1 << track) )
      t->state.SYNC_MEASURE = 1;

  MIOS32_IRQ_Enable();

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Used by the transposer to request the next step in "step trigger" mode
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_StepTriggerReq(u8 bus)
{
  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  seq_cc_trk_t *tcc = &seq_cc_trk[0];

  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t, ++tcc) {
    if( tcc->busasg.bus == bus ) {
      t->state.TRIGGER_NEXT_STEP_REQ = 1;
    }
  }
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function is called by the "Live" function on incoming MIDI events,
// currently we use it to control the temporary layer mutes.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_NotifyIncomingMIDIEvent(u8 track, mios32_midi_package_t p)
{
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // invalid track

  if( !seq_core_options.LIVE_LAYER_MUTE_STEPS )
    return 0; // disabled

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum )
    return 0; // (currently) not relevant in drum mode

  switch( p.event ) {
  //case NoteOff:
  case NoteOn: {
    if( p.velocity == 0 ) // ignore Note Offs
      break;

    // temporary mute layers which are assigned to notes or chords
    u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
    int par_layer;
    int num_p_layers = SEQ_PAR_NumLayersGet(track);
    u16 mask = 1;
    for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr, mask <<= 1) {
      if( *layer_type_ptr == SEQ_PAR_Type_Note || *layer_type_ptr == SEQ_PAR_Type_Chord1 || *layer_type_ptr == SEQ_PAR_Type_Chord2 || *layer_type_ptr == SEQ_PAR_Type_Chord3 ) {
	// hm... should we also play a note off for active notes?
	// and should we mute the sequencer notes as long as no Note Off has been played?
	// problem: we would have to track all actively played MIDI notes, this consumes a lot of memory

	portENTER_CRITICAL();
	if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 1 ) {
	  t->layer_muted |= mask;      // mute layer immediately
	} else {
	  t->layer_muted_from_midi |= mask;      // mute layer immediately
	  t->layer_muted_from_midi_next |= mask; // and for the next step
	}
	portEXIT_CRITICAL();
      }
    }
  } break;

  //case PolyPressure:
  case CC:
  case ProgramChange:
  //case Aftertouch:
  case PitchBend: {
    // temporary mute layers which are assigned to the corresponding event
    u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
    int par_layer;
    int num_p_layers = SEQ_PAR_NumLayersGet(track);
    u16 mask = 1;
    for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr, mask <<= 1) {
      u8 apply_mask = 0;
      switch( *layer_type_ptr ) {
      case SEQ_PAR_Type_CC: {
	if( p.event == CC && p.cc_number == tcc->lay_const[1*16 + par_layer] ) {
	  apply_mask = 1;
	}
      } break;

      case SEQ_PAR_Type_PitchBend: {
	if( p.event == PitchBend ) {
	  apply_mask = 1;
	}
      } break;

      case SEQ_PAR_Type_ProgramChange: {
	if( p.event == ProgramChange ) {
	  apply_mask = 1;
	}
      } break;
      }

      if( apply_mask ) {
	portENTER_CRITICAL();
	if( seq_core_options.LIVE_LAYER_MUTE_STEPS == 1 ) {
	  t->layer_muted |= mask;      // mute layer immediately
	} else {
	  t->layer_muted_from_midi |= mask;      // mute layer immediately
	  t->layer_muted_from_midi_next |= mask; // and for the next step
	}
	portEXIT_CRITICAL();
      }
    }

    // check also LFO CC (note: only handled as temporary change)
    if( p.event == CC && p.cc_number == tcc->lfo_cc ) {
      portENTER_CRITICAL();
      t->lfo_cc_muted_from_midi = 1;
      t->lfo_cc_muted_from_midi_next = 1;
      portEXIT_CRITICAL();
    }

  } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function requests to pre-generate sequencer ticks for a given time
// It returns -1 if the previously requested delay hasn't passed yet
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_AddForwardDelay(u16 delay_ms)
{
  if( bpm_tick_prefetch_req )
    return -1; // ongoing request

  // calculate how many BPM ticks have to be forwarded
  u32 delay_ticks = SEQ_BPM_TicksFor_mS(delay_ms);
  bpm_tick_prefetch_req = SEQ_BPM_TickGet() + delay_ticks;

  return delay_ticks; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function updates the BPM rate in a given sweep time
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_BPM_Update(float bpm, float sweep_ramp)
{
  if( sweep_ramp <= 0.0 ) {
    seq_core_bpm_target = bpm;
    SEQ_BPM_Set(seq_core_bpm_target);
    SEQ_MIDI_PORT_ClkDelayUpdateAll();
    seq_core_bpm_sweep_inc = 0.0;
  } else {
    seq_core_bpm_target = bpm;
    seq_core_bpm_sweep_inc = (seq_core_bpm_target - SEQ_BPM_Get()) / (10.0 * sweep_ramp);
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function should be called each mS to update the BPM
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_BPM_SweepHandler(void)
{
  static u8 prescaler = 0;

  // next step each 100 mS
  if( ++prescaler < 100 )
    return 0;
  prescaler = 0;

  if( seq_core_bpm_sweep_inc != 0.0 ) {
    float current_bpm = SEQ_BPM_Get();
    float tolerance = 0.1;

    if( (seq_core_bpm_sweep_inc > 0.0 && current_bpm >= (seq_core_bpm_target-tolerance)) ||
	(seq_core_bpm_sweep_inc < 0.0 && current_bpm <= (seq_core_bpm_target+tolerance)) ) {
      seq_core_bpm_sweep_inc = 0.0; // final value reached
      SEQ_BPM_Set(seq_core_bpm_target);
      SEQ_MIDI_PORT_ClkDelayUpdateAll();
    } else {
      SEQ_BPM_Set(current_bpm + seq_core_bpm_sweep_inc);
      SEQ_MIDI_PORT_ClkDelayUpdateAll();
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Scrub function called from UI when SCRUB button pressed and Datawheel
// is moved
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Scrub(s32 incrementer)
{
  // simple but useful: increment/decrement the step of each track
  // (in MBSEQ V3 we only generated some additional clocks, which had
  // the disadvantage, that sequences couldn't be scrubbed back)

  // this simplified method currently has following disadvantages:
  // - clock dividers not taken into account (difficult, needs some code restructuring in SEQ_CORE_Tick())
  // - ...and?
  // advantage:
  // - sequencer stays in sync with outgoing/incoming MIDI clock!
  // - reverse scrubbing for some interesting effects while played live (MB-808 has a similar function: nudge)

  u8 track;
  seq_core_trk_t *t = &seq_core_trk[0];
  seq_cc_trk_t *tcc = &seq_cc_trk[0];
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t, ++tcc) {
    SEQ_CORE_NextStep(t, tcc, 0, incrementer >= 0 ? 0 : 1);
  }

#if 0
  // disabled so that we stay in Sync with MIDI clock!
  // increment/decrement reference step
  if( incrementer >= 0 ) {
    if( ++seq_core_state.ref_step > seq_core_steps_per_measure )
      seq_core_state.ref_step = 0;
  } else {
    if( seq_core_state.ref_step )
      --seq_core_state.ref_step;
    else
      seq_core_state.ref_step = seq_core_steps_per_measure;
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function has to be called from the UI whenever notes have been
// updated to ensure that an ongoing sustained note is cancled if
// there is no step played by the track anymore.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_CancelSustainedNotes(u8 track)
{
  if( track >= SEQ_CORE_NUM_TRACKS )
    return -1; // invalid track

  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( t->state.SUSTAINED && tcc->event_mode != SEQ_EVENT_MODE_Drum ) {
    u8 gate_trg_assignment = tcc->trg_assignments.gate;

    if( gate_trg_assignment ) {
      u8 any_gate_set = 0;
      u8 trg_instrument = 0;
      int trk_len = (int)tcc->length + 1;
      int i;

      for(i=0; i<trk_len; ++i) {
	if( SEQ_TRG_Get(track, i, gate_trg_assignment-1, trg_instrument) ) {
	  any_gate_set = 1;
	  break;
	}
      }

      if( !any_gate_set ) {
	t->state.CANCEL_SUSTAIN_REQ = 1;
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function ensures, that a (transposed) note is within
// the <lower>..<upper> range.
//
// If the note is outside the range, it will be "trimmed" in the semitone
// range, and the octave will be kept.
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_CORE_TrimNote(s32 note, u8 lower, u8 upper)
{
  // negative note (e.g. after transpose?)
  // shift it to the positive range
  if( note < 0 )
    note = 11 - ((-note - 1) % 12);

  // check for lower boundary
  if( note < (s32)lower ) {
    note = 12*(lower/12) + (note % 12);
  }

  // check for upper boundary
  if( note > (s32)upper ) {
    note = 12*(upper/12) + (note % 12);

    // if note still > upper value (e.g. upper is set to >= 120)
    // an if (instead of while) should work in all cases, because note will be (12*int(127/12)) + 11 = 131 in worst case!
    if( note > upper )
      note -= 12;
  }

  return note;
}
