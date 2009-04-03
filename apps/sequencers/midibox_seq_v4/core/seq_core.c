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

#include "seq_core.h"
#include "seq_random.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_scale.h"
#include "seq_groove.h"
#include "seq_humanize.h"
#include "seq_midi_in.h"
#include "seq_midi_router.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_pattern.h"
#include "seq_song.h"
#include "seq_random.h"
#include "seq_record.h"
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
#define STOPWATCH_PERFORMANCE_MEASURING 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SEQ_CORE_PlayOffEvents(void);
static s32 SEQ_CORE_Tick(u32 bpm_tick);

static s32 SEQ_CORE_ResetTrkPos(seq_core_trk_t *t, seq_cc_trk_t *tcc);
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, s32 reverse);
static s32 SEQ_CORE_Transpose(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *p);
static s32 SEQ_CORE_Echo(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t p, u32 bpm_tick, u32 gatelength);


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

seq_core_options_t seq_core_options;
u8 seq_core_steps_per_measure;

u8 seq_core_global_scale;
u8 seq_core_global_scale_ctrl;
u8 seq_core_global_scale_root;
u8 seq_core_global_scale_root_selection;
u8 seq_core_keyb_scale_root;

u8 seq_core_bpm_div_int;
u8 seq_core_bpm_div_ext;

seq_core_state_t seq_core_state;
seq_core_trk_t seq_core_trk[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 bpm_tick_prefetch_req;
static u32 bpm_tick_prefetched_ctr;


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Init(u32 mode)
{
  seq_core_options.ALL = 0;
  seq_core_steps_per_measure = 16-1;
  seq_core_global_scale = 0;
  seq_core_global_scale_ctrl = 0; // global
  seq_core_global_scale_root_selection = 0; // from keyboard
  seq_core_keyb_scale_root = 0; // taken if enabled in OPT menu
  seq_core_bpm_div_int = 0; // (frequently used, therefore not part of seq_core_options union)
  seq_core_bpm_div_ext = 1;

  seq_core_state.ALL = 0;

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

  // reset humanizer module
  SEQ_HUMANIZE_Init(0);

  // reset song module
  SEQ_SONG_Init(0);

  // clear registers which are not reset by SEQ_CORE_Reset()
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];

    // if track name only contains spaces, the UI will print 
    // the track number instead of an empty message
    // this ensures highest flexibility (e.g. any track can 
    // play patterns normaly assigned for other tracks w/o inconsistent messages)
    // -> see SEQ_LCD_PrintTrackName()
    int i;
    for(i=0; i<80; ++i)
      seq_core_trk[track].name[i] = ' ';
    seq_core_trk[track].name[80] = 0;
  }

  // reset sequencer
  SEQ_CORE_Reset();

  // init BPM generator
  SEQ_BPM_Init(0);

  SEQ_BPM_PPQN_Set(384);
  SEQ_BPM_Set(140.0);

#if STOPWATCH_PERFORMANCE_MEASURING
  SEQ_UI_MENU_StopwatchInit();
#endif

  return 0; // no error
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
    // so long any Stop/Cont/Start/SongPos event hasn't been flagged to the sequencer
    if( SEQ_BPM_ChkReqStop() ) {
      SEQ_ROUTER_SendMIDIClockEvent(0xfc, 0);
      SEQ_CORE_PlayOffEvents();
    }

    if( SEQ_BPM_ChkReqCont() ) {
      SEQ_ROUTER_SendMIDIClockEvent(0xfb, 0);
      // release pause mode
      ui_seq_pause = 0;
    }

    if( SEQ_BPM_ChkReqStart() ) {
      SEQ_ROUTER_SendMIDIClockEvent(0xfa, 0);
      SEQ_SONG_Reset();
      SEQ_CORE_Reset();
    }

    u16 new_song_pos;
    if( SEQ_BPM_ChkReqSongPos(&new_song_pos) ) {
      SEQ_CORE_PlayOffEvents();
      // cancel prefetch requests/counter
      bpm_tick_prefetch_req = 0;
      bpm_tick_prefetched_ctr = 0;

      SEQ_SONG_Reset();
      // TODO: set new song position

      seq_core_state.FIRST_CLK = 1;
      int track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	seq_core_trk_t *t = &seq_core_trk[track];
	seq_cc_trk_t *tcc = &seq_cc_trk[track];
	SEQ_CORE_ResetTrkPos(t, tcc);
      }      
    }

    u32 bpm_tick;
    if( SEQ_BPM_ChkReqClk(&bpm_tick) > 0 ) {
      again = 1; // check all requests again after execution of this part

      if( bpm_tick_prefetched_ctr ) {
	// ticks already generated due to prefetching - wait until we catch up
	--bpm_tick_prefetched_ctr;
      } else if( bpm_tick_prefetch_req ) {
	// forwarding the sequencer has been requested (e.g. before pattern change)
	u32 forwarded_bpm_tick = bpm_tick;

#if LED_PERFORMANCE_MEASURING == 2
	MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 2
	SEQ_UI_MENU_StopwatchReset();
#endif
	while( bpm_tick_prefetch_req > forwarded_bpm_tick ) {
	  SEQ_CORE_Tick(forwarded_bpm_tick);
	  ++forwarded_bpm_tick;
	  ++bpm_tick_prefetched_ctr;
	}
	bpm_tick_prefetch_req = 0;
#if LED_PERFORMANCE_MEASURING == 2
	MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 2
	SEQ_UI_MENU_StopwatchCapture();
#endif
      } else {
	// realtime generation of events
#if LED_PERFORMANCE_MEASURING == 1
	MIOS32_BOARD_LED_Set(0xffffffff, 1);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 1
	SEQ_UI_MENU_StopwatchReset();
#endif
	SEQ_CORE_Tick(bpm_tick);
#if LED_PERFORMANCE_MEASURING == 1
	MIOS32_BOARD_LED_Set(0xffffffff, 0);
#endif
#if STOPWATCH_PERFORMANCE_MEASURING == 1
	SEQ_UI_MENU_StopwatchCapture();
#endif
      }
    }
  } while( again && num_loops < 10 );

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function plays all "off" events
// Should be called on sequencer reset/restart/pause to avoid hanging notes
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_PlayOffEvents(void)
{
  // play "off events"
  SEQ_MIDI_OUT_FlushQueue();

  // clear sustain/stretch flags
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    t->state.SUSTAINED = 0;
    t->state.STRETCHED_GL = 0;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets song position of sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_Reset(void)
{
  ui_seq_pause = 0;
  seq_core_state.FIRST_CLK = 1;

  int track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    seq_core_trk_t *t = &seq_core_trk[track];
    seq_cc_trk_t *tcc = &seq_cc_trk[track];

    u8 muted = t->state.MUTED; // save muted flag
    t->state.ALL = 0;
    t->state.MUTED = muted; // restore muted flag
    SEQ_CORE_ResetTrkPos(t, tcc);
  }

  // since timebase has been changed, ensure that Off-Events are played 
  // (otherwise they will be played much later...)
  SEQ_CORE_PlayOffEvents();

  // reset BPM tick
  SEQ_BPM_TickSet(0);

  // cancel prefetch requests/counter
  bpm_tick_prefetch_req = 0;
  bpm_tick_prefetched_ctr = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// performs a single ppqn tick
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_Tick(u32 bpm_tick)
{
#if 0
  // handle external clock divider
  if( !(bpm_tick % (1 << (seq_core_bpm_div_ext+1))) ) {
    // TODO: send external clock
  }
#endif

  // increment reference step on each 16th note
  // set request flag on overrun (tracks can synch to measure)
  u8 synch_to_measure_req = 0;
  if( (bpm_tick % ((384/4) << seq_core_bpm_div_int)) == 0 ) {
    if( seq_core_state.FIRST_CLK )
      seq_core_state.ref_step = 0;
    else {
      if( ++seq_core_state.ref_step > seq_core_steps_per_measure ) {
	seq_core_state.ref_step = 0;
	synch_to_measure_req = 1;
      }
    }
  }

  // send MIDI clock on each 16th tick (since we are working at 384ppqn)
  if( (bpm_tick % (16 << seq_core_bpm_div_int)) == 0 )
    SEQ_ROUTER_SendMIDIClockEvent(0xf8, bpm_tick);


  // process all tracks
  // first the loopback ports, thereafter parameters sent to common MIDI ports
  int round;
  for(round=0; round<2; ++round) {
    seq_core_trk_t *t = &seq_core_trk[0];
    seq_cc_trk_t *tcc = &seq_cc_trk[0];
    int track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++t, ++tcc, ++track) {

      // round 0: loopback ports, round 1: remaining ports
      u8 loopback_port = (tcc->midi_port & 0xf0) == 0xf0;
      if( (!round && !loopback_port) || (round && loopback_port) )
	continue;

      // sustained note: play off event if sustain mode has been disabled and no stretched gatelength
      if( !tcc->mode.SUSTAIN && !t->state.STRETCHED_GL && t->state.SUSTAINED ) {
	SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick);
	t->state.SUSTAINED = 0;
      }

      // if "synch to measure" flag set: reset track if master has reached the selected number of steps
      // MEMO: we could also provide the option to synch to another track
      if( synch_to_measure_req && tcc->clkdiv.SYNCH_TO_MEASURE )
        SEQ_CORE_ResetTrkPos(t, tcc);
  
      if( t->state.FIRST_CLK || bpm_tick >= t->timestamp_next_step ) {
	// calculate step length
	u16 step_length_pre = ((tcc->clkdiv.value+1) * (tcc->clkdiv.TRIPLETS ? 4 : 6));
        t->step_length = step_length_pre << seq_core_bpm_div_int;

	// set timestamp of next step w/o groove delay (reference timestamp)
	if( t->state.FIRST_CLK )
	  t->timestamp_next_step_ref = bpm_tick + t->step_length;
	else
	  t->timestamp_next_step_ref += t->step_length;

        // increment step if not in arpeggiator mode or arp position == 0
        if( tcc->mode.playmode != SEQ_CORE_TRKMODE_Arpeggiator || !t->arp_pos ) {
	  u8 skip_ctr = 0;
	  do {
	    // determine next step depending on direction mode
	    if( !t->state.FIRST_CLK ) {
	      SEQ_CORE_NextStep(t, tcc, 0); // 0=not reverse
	    }
	    
	    // clear "first clock" flag (on following clock ticks we can continue as usual)
	    t->state.FIRST_CLK = 0;
  
	    // if skip flag set for this flag: try again
	    if( SEQ_TRG_SkipGet(track, t->step, 0) )
	      ++skip_ctr;
	    else
	      break;
	    
	  } while( skip_ctr < 32 ); // try 32 times maximum

	  // calculate number of cycles to next step
	  t->timestamp_next_step = t->timestamp_next_step_ref + (SEQ_GROOVE_DelayGet(track, t->step + 1) << seq_core_bpm_div_int);
        }

        // solo function: don't play MIDI event if track not selected
        // mute function
        // track disabled
        if( (seq_ui_button_state.SOLO && !SEQ_UI_IsSelectedTrack(track)) ||
	    t->state.MUTED || // Mute function
	    tcc->mode.playmode == SEQ_CORE_TRKMODE_Off ) { // track disabled
	  if( t->state.STRETCHED_GL || t->state.SUSTAINED ) {
	    SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick);
	    t->state.STRETCHED_GL = 0;
	    t->state.SUSTAINED = 0;
	  }
	  continue;
	}
  
  
        // if random gate trigger set: play step with 1:1 probability
        if( SEQ_TRG_RandomGateGet(track, t->step, 0) && (SEQ_RANDOM_Gen(0) & 1) )
	  continue;

	// if not in drum mode (only checked once)
	// if probability < 100: play step with given probability
	if( tcc->event_mode != SEQ_EVENT_MODE_Drum ) {
	  u8 rnd_probability;
	  if( (rnd_probability=SEQ_PAR_ProbabilityGet(track, t->step, 0)) < 100 &&
	      SEQ_RANDOM_Gen_Range(0, 99) >= rnd_probability )
	    continue;
	}
  
        // fetch MIDI events which should be played
        seq_layer_evnt_t layer_events[16];
        s32 number_of_events = SEQ_LAYER_GetEvents(track, t->step, layer_events);
        if( number_of_events > 0 ) {
	  int i;
          seq_layer_evnt_t *e = &layer_events[0];
          for(i=0; i<number_of_events; ++e, ++i) {
            mios32_midi_package_t *p = &e->midi_package;

	    // tag for scheduled events
	    p->cable = track;

	    // instrument layers only used for drum tracks
	    u8 instrument = (tcc->event_mode == SEQ_EVENT_MODE_Drum) ? i : 0;

	    // individual for each instrument in drum mode:
	    // if probability < 100: play step with given probability
	    if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
	      u8 rnd_probability;
	      if( (rnd_probability=SEQ_PAR_ProbabilityGet(track, t->step, instrument)) < 100 &&
		  SEQ_RANDOM_Gen_Range(0, 99) >= rnd_probability )
		continue;
	    }
  
            // transpose notes/CCs
            SEQ_CORE_Transpose(t, tcc, p);


            // skip if velocity has been cleared by transpose function
            // (e.g. no key pressed in transpose mode)
            if( p->type == NoteOn && !p->velocity ) {
	      // stretched note, length < 96: queue off event
	      if( t->state.STRETCHED_GL && t->state.SUSTAINED && (e->len < 96) ) {
		SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick + t->bpm_tick_delay);
		t->state.SUSTAINED = 0;
		t->state.STRETCHED_GL = 0;
	      }
	      continue;
	    }

  
            // force to scale
            if( tcc->mode.FORCE_SCALE ) {
	      u8 scale, root_selection, root;
	      SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);
	      SEQ_SCALE_Note(p, scale, root);
            }

            // glide trigger
            if( e->len > 0 ) {
	      if( SEQ_TRG_GlideGet(track, t->step, instrument) )
		e->len = 96; // Glide
            }

	    // get delay of step (0..95)
	    // note: negative delays stored in step parameters would require to pre-generate bpm_ticks, 
	    // which would reduce the immediate response on value/trigger changes
	    // therefore negative delays are only supported for groove patterns, and they are
	    // applied over the whole track (e.g. drum mode: all instruments of the appr. track)
	    u16 prev_bpm_tick_delay = t->bpm_tick_delay;
	    t->bpm_tick_delay = SEQ_PAR_StepDelayGet(track, t->step, instrument);

	    // scale delay (0..95) over next clock counter to consider the selected clock divider
	    if( t->bpm_tick_delay )
	      t->bpm_tick_delay = (t->bpm_tick_delay * t->step_length) / 96;


            // determine gate length
            u32 gatelength = 0;
            u8 triggers = 0;

            if( p->type == CC || p->type == PitchBend ) {
	      // CC/Pitchbend w/o gatelength, just play it
	      triggers = 1;
            } else if( p->note && p->velocity && (e->len >= 0) ) {
	      // Any other event with gatelength

	      // groove it
	      SEQ_GROOVE_Event(track, t->step, e);

	      // humanize it
	      SEQ_HUMANIZE_Event(track, t->step, e);

	      // sustained or stretched note: play off event of previous step
	      if( t->state.SUSTAINED ) {
		SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick + prev_bpm_tick_delay);
		t->state.SUSTAINED = 0;
		t->state.STRETCHED_GL = 0;
	      }

	      if( tcc->mode.SUSTAIN || e->len >= 96 ) {
		// sustained note: play note at timestamp, and queue off event at 0xffffffff (so that it can be re-scheduled)
		t->vu_meter = p->velocity; // for visualisation in mute menu
		SEQ_MIDI_OUT_Send(tcc->midi_port, *p, SEQ_MIDI_OUT_OnEvent, bpm_tick + t->bpm_tick_delay, 0);
		p->velocity = 0;
		SEQ_MIDI_OUT_Send(tcc->midi_port, *p, SEQ_MIDI_OUT_OffEvent, 0xffffffff, 0);

		// notify stretched gatelength if not in sustain mode
		t->state.SUSTAINED = 1;
		if( !tcc->mode.SUSTAIN )
		  t->state.STRETCHED_GL = 1;
		// triggers/gatelength already 0 - don't queue additional notes
	      } else {
		triggers = 1;
		gatelength = e->len;
	      }
            } else if( t->state.STRETCHED_GL && t->state.SUSTAINED && (e->len < 96) ) {
	      // stretched note, length < 96: queue off events
	      SEQ_MIDI_OUT_ReSchedule(track, SEQ_MIDI_OUT_OffEvent, bpm_tick + prev_bpm_tick_delay);
	      t->state.SUSTAINED = 0;
	      t->state.STRETCHED_GL = 0;
	    }


	    // roll/flam?
	    u8 roll_mode;
	    if( triggers ) {
	      // get roll mode from parameter layer
	      roll_mode=SEQ_PAR_RollModeGet(track, t->step, instrument);
	      // with less priority (parameter == 0): force roll mode if Roll trigger is set
	      if( !roll_mode && SEQ_TRG_RollGet(track, t->step, instrument) )
		roll_mode = 0x16; // 3D06
	      // if roll mode != 0: increase number of triggers
	      if( roll_mode )
		triggers = ((roll_mode & 0x30)>>4) + 2;
	    }

            
            // schedule events
            if( triggers ) {
	      if( p->type == CC || p->type == PitchBend ) {
		SEQ_MIDI_OUT_Send(tcc->midi_port, *p, SEQ_MIDI_OUT_CCEvent, bpm_tick + t->bpm_tick_delay, 0);
		t->vu_meter = 0x7f; // for visualisation in mute menu
	      } else {
		// force velocity to 0x7f (drum mode: selectable value) if accent flag set
		if( SEQ_TRG_AccentGet(track, t->step, instrument) ) {
		  if( tcc->event_mode == SEQ_EVENT_MODE_Drum )
		    p->velocity = tcc->lay_const[2*16 + i];
		  else
		    p->velocity = 0x7f;
		}

		t->vu_meter = p->velocity; // for visualisation in mute menu
		
		if( loopback_port ) {
		  // forward to MIDI IN handler immediately
		  SEQ_MIDI_IN_Receive(tcc->midi_port, *p);
		  // multi triggers, but also echo not possible on loopback ports
		} else {
		  if( triggers > 1 ) {
		    int i;
#if 1
		    // force gatelength depending on number of triggers
		    if( triggers < 4 ) {
		      //   number of triggers:    2   3   4   5
		      const u8 gatelength_tab[4] = { 48, 32, 36, 32 };
		      // strategy:
		      // 2 triggers: played within 1 step at 0 and 48
		      // 3 triggers: played within 1 step at 0, 32 and 64
		      // 4 triggers: played within 1.5 steps at 0, 36, 72 and 108
		      // 5 triggers: played within 1.5 steps at 0, 32, 64, 96 and 128

		      // in addition, scale length (0..95) over next clock counter to consider the selected clock divider
		      gatelength = (gatelength_tab[triggers-2] * t->step_length) / 96;
		    }
#endif

		    u32 half_gatelength = gatelength/2;
		    if( !half_gatelength )
		      half_gatelength = 1;
      	      
		    mios32_midi_package_t p_multi = *p;
		    u16 roll_attenuation = 256 - (2 * triggers * (16 - (roll_mode & 0x0f))); // magic formula for nice effects
		    if( roll_mode & 0x40 ) { // upwards
		      for(i=triggers-1; i>=0; --i) {
			SEQ_MIDI_OUT_Send(tcc->midi_port, p_multi, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay + i*gatelength, half_gatelength);
			u16 velocity = roll_attenuation * p_multi.velocity;
			p_multi.velocity = velocity >> 8;
		      }
		    } else { // downwards
		      for(i=0; i<triggers; ++i) {
			SEQ_MIDI_OUT_Send(tcc->midi_port, p_multi, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay + i*gatelength, half_gatelength);
			if( roll_mode ) {
			  u16 velocity = roll_attenuation * p_multi.velocity;
			  p_multi.velocity = velocity >> 8;
			}
		      }
		    }
		  } else {
		    if( !gatelength )
		      gatelength = 1;
		    else // scale length (0..95) over next clock counter to consider the selected clock divider
		      gatelength = (gatelength * t->step_length) / 96;
		    SEQ_MIDI_OUT_Send(tcc->midi_port, *p, SEQ_MIDI_OUT_OnOffEvent, bpm_tick + t->bpm_tick_delay, gatelength);
		  }		    
		}

		// apply Fx if "No Fx" trigger is not set
		if( !SEQ_TRG_NoFxGet(track, t->step, instrument) ) {
		  // apply echo Fx if enabled
		  if( tcc->echo_repeats && (p->type == CC || p->type == PitchBend || gatelength) )
		    SEQ_CORE_Echo(t, tcc, *p, bpm_tick + t->bpm_tick_delay, gatelength);
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

  // in song mode:
  // increment song position shortly before we reset the reference step
  // TODO: implement prefetching until end of step!
  if( SEQ_SONG_ActiveGet() &&
      seq_core_state.ref_step == seq_core_steps_per_measure &&
      (bpm_tick % (96 << seq_core_bpm_div_int)) == 0 ) {
    SEQ_SONG_NextPos();
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Resets the step position variables of a track
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_ResetTrkPos(seq_core_trk_t *t, seq_cc_trk_t *tcc)
{
  // don't increment on first clock event
  t->state.FIRST_CLK = 1;

  // clear delay
  t->bpm_tick_delay = 0;

  // reset step REPLAY and FWD counters
  t->step_replay_ctr = 0;
  t->step_fwd_ctr = 0;

  // reset record state and other record specific variables
  if( ui_page = SEQ_UI_PAGE_TRKREC ) {
    t->state.REC_EVENT_ACTIVE = 0;
    t->state.REC_MUTE_NEXT_STEP = 0;
    SEQ_RECORD_Reset();
  }

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

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Determine next step depending on direction mode
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_NextStep(seq_core_trk_t *t, seq_cc_trk_t *tcc, s32 reverse)
{
  int i;
  u8 save_step = 0;
  u8 new_step = 1;

  // handle progression parameters if position shouldn't be reset
  if( !reverse && !t->state.POS_RESET ) {
    if( ++t->step_fwd_ctr > tcc->steps_forward ) {
      t->step_fwd_ctr = 0;
      if( tcc->steps_jump_back ) {
	for(i=0; i<tcc->steps_jump_back; ++i) {
	  SEQ_CORE_NextStep(t, tcc, 1); // 1=reverse
	}
      }
      if( ++t->step_replay_ctr > tcc->steps_replay ) {
	t->step_replay_ctr = 0;
	save_step = 1; // request to save the step in t->step_saved at the end of this routine
      } else {
	t->step = t->step_saved;
	new_step = 0; // don't calculate new step
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
static s32 SEQ_CORE_Transpose(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t *p)
{
  int note = p->note;

  int inc_oct = tcc->transpose_oct;
  if( inc_oct >= 8 )
    inc_oct -= 16;

  int inc_semi = tcc->transpose_semi;
  if( inc_semi >= 8 )
    inc_semi -= 16;

  // in transpose or arp playmode we allow to transpose notes and CCs
  if( tcc->mode.playmode == SEQ_CORE_TRKMODE_Transpose ) {
    int tr_note = SEQ_MIDI_IN_TransposerNoteGet(tcc->mode.HOLD);

    if( tr_note < 0 ) {
      p->velocity = 0; // disable note and exit
      return -1; // note has been disabled
    }

    inc_semi += tr_note - 0x3c; // C-3 is the base note
  }
  else if( tcc->mode.playmode == SEQ_CORE_TRKMODE_Arpeggiator ) {
    int key_num = (note >> 2) & 0x3;
    int arp_oct = (note >> 4) & 0x7;

    if( arp_oct < 2 ) { // Multi Arp Event
      inc_oct += ((note >> 2) & 7) - 4;
      key_num = t->arp_pos;
    } else {
      inc_oct += arp_oct - 4;
    }

    int arp_note = SEQ_MIDI_IN_ArpNoteGet(tcc->mode.HOLD, !tcc->mode.UNSORTED, key_num);

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
  }

#if 0
  // apply transpose octave/semitones parameter
  if( p->type != NoteOn && p->type != NoteOff )
    return -1; // no note event
  // TODO: what about tracks which play transposed/arp events + CC?
#endif

  if( inc_oct ) {
    note += 12 * inc_oct;
    if( inc_oct < 0 ) {
      while( note < 0 )
	note += 12;
    } else {
      while( note >= 128 )
	note -= 12;
    }
  }

  if( inc_semi ) {
    note += inc_semi;
    if( inc_semi < 0 ) {
      while( note < 0 )
	note += 12;
    } else {
      while( note >= 128 )
	note -= 12;
    }
  }

  p->note = note;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns the selected scale and root note selection depending on
// global/group specific settings
// scale and root note are for interest while playing the sequence -> SEQ_CORE
// scale and root selection are for interest when editing the settings -> SEQ_UI_OPT
// Both functions are calling this function to ensure consistency
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_CORE_FTS_GetScaleAndRoot(u8 *scale, u8 *root_selection, u8 *root)
{
  if( seq_core_global_scale_ctrl > 0 ) {
    // scale/root selection from a specific pattern group
    u8 group = seq_core_global_scale_ctrl-1;
    *scale = seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+2].shared.scale;
    *root_selection = seq_cc_trk[(group*SEQ_CORE_NUM_TRACKS_PER_GROUP)+3].shared.scale_root;
  } else {
    // global scale/root selection
    *scale = seq_core_global_scale;
    *root_selection = seq_core_global_scale_root_selection;
  }
  *root = (*root_selection == 0) ? seq_core_keyb_scale_root : (*root_selection-1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Echo Fx
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_CORE_Echo(seq_core_trk_t *t, seq_cc_trk_t *tcc, mios32_midi_package_t p, u32 bpm_tick, u32 gatelength)
{
  // thanks to MIDI queuing mechanism, this is a no-brainer :)

  // 64T, 64, 32T, 32, 16T, 16, ... 1, Rnd1 and Rnd2
  s32 fb_ticks = ((tcc->echo_delay & 1) ? 24 : 16) * (1 << (tcc->echo_delay>>1));

  s32 fb_note = p.note;
  s32 fb_note_base = fb_note; // for random function

  // TODO: echo port and MIDI channel
  
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
  if( (p.type == CC || p.type == PitchBend) && !gatelength )
    event_type = SEQ_MIDI_OUT_CCEvent;

  // for the case that force-to-scale is activated
  u8 scale, root_selection, root;
  SEQ_CORE_FTS_GetScaleAndRoot(&scale, &root_selection, &root);

  u32 echo_offset = fb_ticks;
  int i;
  for(i=0; i<tcc->echo_repeats; ++i) {
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
      while( fb_note < 0 )
	fb_note += 12;
      while( fb_note > 127 )
	fb_note -= 12;

      p.note = (u8)fb_note;
    }

    if( gatelength && tcc->echo_fb_gatelength != 20 ) { // 20 == 100% -> no change
      gatelength = (gatelength * 5*tcc->echo_fb_gatelength) / 100;
      if( !gatelength )
	gatelength = 1;
    }

    // force to scale
    if( tcc->mode.FORCE_SCALE ) {
      SEQ_SCALE_Note(&p, scale, root);
    }

    SEQ_MIDI_OUT_Send(tcc->midi_port, p, event_type, bpm_tick + echo_offset, gatelength);
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
  bpm_tick_prefetch_req = SEQ_BPM_TickGet() + SEQ_BPM_TicksFor_mS(delay_ms);

  return 0; // no error
}
