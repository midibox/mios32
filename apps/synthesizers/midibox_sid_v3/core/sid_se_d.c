// $Id$
/*
 * MBSID Drum Engine
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
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

#include <sid.h>

#include "sid_se.h"
#include "sid_se_d.h"
#include "sid_patch.h"
#include "sid_voice.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 SID_SE_D_Pitch(sid_se_voice_t *v);
static s32 SID_SE_D_PW(sid_se_voice_t *v);
static s32 SID_SE_D_WT(sid_se_wt_t *w);
static s32 SID_SE_D_Seq(sid_se_seq_t *s);


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_D_Update(u8 sid)
{
  int i;
  sid_se_vars_t *vars = &sid_se_vars[sid];


  ///////////////////////////////////////////////////////////////////////////
  // Clear all modulation destinations
  ///////////////////////////////////////////////////////////////////////////

  s32 *mod_dst_array_clr = sid_se_vars[sid].mod_dst;
  for(i=0; i<SID_SE_NUM_MOD_DST; ++i)
    *mod_dst_array_clr++ = 0; // faster than memset()! (ca. 20 uS) - seems that memset only copies byte-wise


  ///////////////////////////////////////////////////////////////////////////
  // Sequencer
  ///////////////////////////////////////////////////////////////////////////

  sid_se_seq_t *s = &sid_se_seq[sid][0];
  SID_SE_D_Seq(s);


  ///////////////////////////////////////////////////////////////////////////
  // Sync requests
  ///////////////////////////////////////////////////////////////////////////
  sid_se_trg_t *trg = &vars->triggers;
  if( trg->ALL[0] ) {
    if( trg->O1L ) SID_SE_NoteRestart(&sid_se_voice[sid][0]);
    if( trg->O2L ) SID_SE_NoteRestart(&sid_se_voice[sid][1]);
    if( trg->O3L ) SID_SE_NoteRestart(&sid_se_voice[sid][2]);
    if( trg->O1R ) SID_SE_NoteRestart(&sid_se_voice[sid][3]);
    if( trg->O2R ) SID_SE_NoteRestart(&sid_se_voice[sid][4]);
    if( trg->O3R ) SID_SE_NoteRestart(&sid_se_voice[sid][5]);
    //if( trg->E1A ) SID_SE_ENV_Restart(&sid_se_env[sid][0]);
    //if( trg->E2A ) SID_SE_ENV_Restart(&sid_se_env[sid][1]);
    trg->ALL[0] = 0;
  }

  // EGs/LFOs not relevant for drum engine
  trg->ALL[1] = 0;

  // done after WTs have been handled
  // trg->ALL[2] = 0;


  ///////////////////////////////////////////////////////////////////////////
  // Wavetables
  ///////////////////////////////////////////////////////////////////////////

  sid_se_wt_t *w = &sid_se_wt[sid][0];
  for(i=0; i<6; ++i, ++w)
    SID_SE_D_WT(w);

  trg->ALL[2] = 0;


  ///////////////////////////////////////////////////////////////////////////
  // Voices
  ///////////////////////////////////////////////////////////////////////////

  // process voices
  sid_se_voice_t *v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    if( SID_SE_Gate(v) > 0 )
      SID_SE_D_Pitch(v);

    // control the delayed gate clear request
    if( v->state.GATE_ACTIVE && !v->set_delay_ctr && v->clr_delay_ctr ) {
      int clr_delay_ctr = v->clr_delay_ctr + sid_se_env_table[v->drum_gatelength] / sid_se_speed_factor;
      if( clr_delay_ctr >= 0xffff ) {
	// overrun: clear counter to disable delay
	v->clr_delay_ctr = 0;
	// request gate clear and deactivate voice active state (new note can be played)
	v->state.GATE_SET_REQ = 0;
	v->state.GATE_CLR_REQ = 1;
	v->state.VOICE_ACTIVE = 0;
      } else
	v->clr_delay_ctr = clr_delay_ctr;
    }

    // Pulsewidth handler
    SID_SE_D_PW(v);
  }


  ///////////////////////////////////////////////////////////////////////////
  // Filters and Volume
  ///////////////////////////////////////////////////////////////////////////
  sid_se_filter_t *f = &sid_se_filter[sid][0];
  SID_SE_FilterAndVolume(f++);
  SID_SE_FilterAndVolume(f);


  ///////////////////////////////////////////////////////////////////////////
  // Tmp: copy register values directly into SID registers
  ///////////////////////////////////////////////////////////////////////////
  v = &sid_se_voice[sid][0];
  for(i=0; i<6; ++i, ++v) {
    // wait until initial gate delay has passed (hard-sync, for ABW feature)
    if( !v->set_delay_ctr ) {
      sid_se_voice_waveform_t waveform = (sid_se_voice_waveform_t)v->drum_waveform;
      v->phys_sid_voice->test = 0;
      v->phys_sid_voice->waveform = waveform.WAVEFORM;
      v->phys_sid_voice->sync = waveform.SYNC;
      v->phys_sid_voice->ringmod = waveform.RINGMOD;

      if( v->assigned_instrument < 16 ) {
	sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[v->sid].D.voice[v->assigned_instrument];

	u8 ad = voice_patch->D.ad;
	u8 sr = voice_patch->D.sr;

	// force sustain to maximum if accent flag active
	if( v->state.ACCENT )
	  sr |= 0xf0;

	v->phys_sid_voice->ad = ad;
	v->phys_sid_voice->sr = sr;
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Pitch Handler for Drums
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_D_Pitch(sid_se_voice_t *v)
{
  // get frequency depending on base note
  u8 note = v->note;
  u8 frq_ix = note + 21;
  if( frq_ix > 127 )
    frq_ix = 127;
  s32 target_frq = sid_se_frq_table[frq_ix];

  if( v->assigned_instrument < 16 ) {
    sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[v->sid].D.voice[v->assigned_instrument];

    // increase/decrease target frequency by pitchrange depending on finetune value
    u8 tune = voice_patch->D.tune;
    if( tune != 0x80 ) {
      u8 pitchrange = 24;
      if( tune > 0x80 ) {
	int dst_note = note + pitchrange + 21;
	if( dst_note > 127 ) dst_note = 127;
	u16 dst_frq = sid_se_frq_table[dst_note];
	target_frq += ((dst_frq-target_frq) * (tune-0x80)) / 128;
	if( target_frq >= 0xffff ) target_frq = 0xffff;
      } else {
	int dst_note = note - pitchrange + 21;
	if( dst_note < 0 ) dst_note = 0;
	u16 dst_frq = sid_se_frq_table[dst_note];
	target_frq -= ((target_frq-dst_frq) * (0x80-tune)) / 128;
	if( target_frq < 0 ) target_frq = 0;
      }
    }
  }

  // copy target frequency into SID registers
  v->phys_sid_voice->frq_l = target_frq & 0xff;
  v->phys_sid_voice->frq_h = target_frq >> 8;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Pulsewidth handler for drums
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_D_PW(sid_se_voice_t *v)
{
  // take pulsewidth specified by drum model
  u16 pulsewidth = v->dm ? (v->dm->pulsewidth << 4) : 0x800;

  // transfer to SID registers
  v->phys_sid_voice->pw_l = pulsewidth & 0xff;
  v->phys_sid_voice->pw_h = (pulsewidth >> 8) & 0x0f;
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Wavetable Sequencer
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_D_WT(sid_se_wt_t *w)
{
  u8 next_step_req = 0;

  // only if drum model is selected
  sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[w->sid][w->wt];
  if( !v->dm )
    return 0;

  // check if WT reset requested
  if( w->trg_src[2] & (1 << w->wt) ) {
    // next clock will increment div to 0
    w->div_ctr = ~0;
    // next step will increment to start position
    w->pos = ~0;
  }

  // clock with each update cycle, so that we are independent from the selected BPM rate
  // increment clock divider
  // reset divider if it already has reached the target value
  if( ++w->div_ctr > (v->drum_wt_speed * sid_se_speed_factor) ) {
    w->div_ctr = 0;
    next_step_req = 1;
  }

  // check for next step request
  // skip if position is 0xaa (notifies oneshot -> WT stopped)
  // wait until initial gate delay has passed (for ABW feature)
  // TK: checking for v->set_delay_ctr here is probably not a good idea,
  // it leads to jitter. However, we let it like it is for compatibility reasons with V2
  if( next_step_req && w->pos != 0xaa && !v->set_delay_ctr ) {
    // increment position counter, reset at end position
    ++w->pos;
    if( v->dm->wavetable[2*w->pos] == 0 ) {
      if( v->dm->wt_loop == 0xff )
	w->pos = 0xaa; // oneshot mode
      else
	w->pos = v->dm->wt_loop;
    }

    if( w->pos != 0xaa ) {
      // "play" the step
      int note = v->dm->wavetable[2*w->pos + 0];
      // transfer to voice
      // if bit #7 of note entry is set: add PAR3/2 and saturate
      if( note & (1 << 7) ) {
	note = (note & 0x7f) + (((int)v->drum_par3 - 0x80) / 2);
	if( note > 127 ) note = 127; else if( note < 0 ) note = 0;
      }
      v->note = note;

      // set waveform
      v->drum_waveform = v->dm->wavetable[2*w->pos + 1];

#if DEBUG_VERBOSE_LEVEL >= 3
      DEBUG_MSG("WT %d: %d (%02x %02x)\n", w->wt, w->pos, v->dm->wavetable[2*w->pos + 0], v->dm->wavetable[2*w->pos + 1]);
#endif
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Drum Sequencer
/////////////////////////////////////////////////////////////////////////////
static s32 SID_SE_D_Seq(sid_se_seq_t *s)
{
  sid_patch_t *patch = (sid_patch_t *)&sid_patch[s->sid];

  // clear gate and deselect sequence if MIDI clock stop requested
  if( sid_se_clk.event.MIDI_STOP ) {
    // stop sequencer
    s->state.RUNNING = 0;
    // clear gates
    SID_SE_D_AllNotesOff(s->sid);

    return 0; // nothing else to do
  }

  // exit if not in sequencer mode
  // clear gates if sequencer just has been disabled (for proper transitions)
  if( !((sid_se_seq_speed_par_t)patch->D.seq_speed).SEQ_ON ) {
    // check if sequencer was disabled before
    if( s->state.ENABLED ) {
      // clear gates
      SID_SE_D_AllNotesOff(s->sid);
    }
    s->state.ENABLED = 0;
    return 0; // nothing else to do
  }

  // check if reset requested for FA event or sequencer was disabled before (transition Seq off->on)
  if( !s->state.ENABLED || sid_se_clk.event.MIDI_START ) {
    // next clock event will increment to 0
    s->div_ctr = ~0;
    s->sub_ctr = ~0;
    // next step will increment to start position
    s->pos = ((patch->D.seq_num & 0x7) << 4) - 1;
  }

  if( sid_se_clk.event.MIDI_START || sid_se_clk.event.MIDI_CONTINUE ) {
    // start sequencer
    s->state.RUNNING = 1;
  }

  // sequencer enabled
  s->state.ENABLED = 1;

  // check for clock sync event
  if( sid_se_clk.event.CLK ) {
    sid_se_seq_speed_par_t speed_par = (sid_se_seq_speed_par_t)patch->D.seq_speed;
    // increment clock divider
    // reset divider if it already has reached the target value
    if( ++s->div_ctr == 0 || s->div_ctr > speed_par.CLKDIV ) {
      s->div_ctr = 0;

      // increment subcounter and check for state
      // 0: new note & gate set
      // 4: gate clear
      // >= 6: reset to 0, new note & gate set
      if( ++s->sub_ctr >= 6 )
	s->sub_ctr = 0;

      if( s->sub_ctr == 0 ) { // set gate
	// increment position counter, reset at end position
	u8 next_step = (s->pos & 0x0f) + 1;
	if( next_step > patch->D.seq_length )
	  next_step = 0;
	else
	  next_step &= 0x0f; // just to ensure...

	// change to new sequence number immediately if SYNCH_TO_MEASURE flag not set, or first step reached
	u8 next_pattern = s->pos >> 4;
	if( !speed_par.SYNCH_TO_MEASURE || next_step == 0 )
	  next_pattern = patch->D.seq_num & 0x7;

	s->pos = (next_pattern << 4) | next_step;

	// play the step

	// gate off if invalid song number (stop feature: seq >= 8)
	if( patch->D.seq_num >= 8 ) {
	  // clear gates
	  SID_SE_D_AllNotesOff(s->sid);
	} else {
          // Sequence Storage - Structure:
          // 4 bytes for 16 steps:
          //  - first byte: [0] gate step #1 ... [7] gate step #8
          //  - second byte: [0] accent step #1 ... [7] accent step #8
          //  - third byte: [0] gate step #9 ... [7] gate step #16
          //  - fourth byte: [0] accent step #9 ... [7] accent step #16
          // 
          // 8 tracks per sequence:
          //  offset 0x00-0x03: track #1
          //  offset 0x04-0x07: track #2
          //  offset 0x08-0x0b: track #3
          //  offset 0x0c-0x0f: track #4
          //  offset 0x00-0x03: track #5
          //  offset 0x04-0x07: track #6
          //  offset 0x08-0x0b: track #7
          //  offset 0x0c-0x0f: track #8
          // 8 sequences:
          //  0x100..0x11f: sequence #1
          //  0x120..0x13f: sequence #2
          //  0x140..0x15f: sequence #3
          //  0x160..0x17f: sequence #4
          //  0x180..0x19f: sequence #5
          //  0x1a0..0x1bf: sequence #6
          //  0x1c0..0x1df: sequence #7
          //  0x1e0..0x1ff: sequence #8
	  u8 *pattern = (u8 *)&patch->D.seq_memory[(s->pos & 0xf0) << 1];

	  // loop through 8 tracks
	  u8 track;
	  u8 step = s->pos & 0x0f;
	  u8 step_offset = (step & (1 << 3)) ? 2 : 0;
	  u8 step_mask = (1 << (step & 0x7));
	  for(track=0; track<8; ++track) {
	    u8 mode = 0;
	    if( pattern[4*track + step_offset + 0] & step_mask )
	      mode |= 1;
	    if( pattern[4*track + step_offset + 1] & step_mask )
	      mode |= 2;

	    // coding:
            // Gate  Accent  Result
            //    0       0  Don't play note
            //    1       0  Play Note w/o accent
            //    0       1  Play Note w/ accent
            //    1       1  Play Secondary Note
	    u8 drum = track*2;
	    u8 gate = 0;
	    u8 velocity = 0x3f; // >= 0x40 selects accent
	    switch( mode ) {
	      case 1: gate = 1; break;
	      case 2: gate = 1; velocity = 0x7f; break;
	      case 3: gate = 1; ++drum; break;
	    }

	    if( gate )
	      SID_SE_D_NoteOn(s->sid, drum, velocity);
	  }
	}
      } else if( s->sub_ctr == 4 ) { // clear gates
	SID_SE_D_AllNotesOff(s->sid);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Note On Handler
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_D_NoteOn(u8 sid, u8 drum, u8 velocity)
{
  int i;

  if( drum >= 16 )
    return -1; // unsupported drum

  // get voice assignment
  sid_se_voice_patch_t *voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].D.voice[drum];
  u8 voice_asg = ((sid_se_v_flags_t)voice_patch->D.v_flags).VOICE_ASG;

  // number of voices depends on mono/stereo mode (TODO: once ensemble available...)
  u8 num_voices = 6;

  // check if drum instrument already played, in this case use the old voice
  sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[sid][0];
  int voice;
  for(voice=0; voice<num_voices; ++voice, ++v) {
    if( v->assigned_instrument == drum )
      break;
  }

  u8 voice_found = 0;
  if( voice < num_voices ) {
    // check if voice assignment still valid (could have been changed meanwhile)
    switch( voice_asg ) {
      case 0: voice_found = 1; break; // All
      case 1: if( voice < 3 ) voice_found = 1; break; // Left Only
      case 2: if( voice >= 3 ) voice_found = 1; break; // Right Only
      default: if( voice == (voice_asg-3) ) voice_found = 1; break; // direct assignment
    }
  }

  // get new voice if required
  if( !voice_found )
    voice = SID_VOICE_Get(sid, drum, voice_asg, num_voices);

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[SID_SE_D_NoteOn:%d] drum %d takes voice %d\n", sid, drum, voice);
#endif

  // get pointer to voice
  v = (sid_se_voice_t *)&sid_se_voice[sid][voice];

  // save instrument number
  v->assigned_instrument = drum;

  // determine drum model and save pointer
  u8 dmodel = voice_patch->D.dmodel;
  if( dmodel >= SID_DMODEL_NUM )
    dmodel = 0;
  v->dm = (sid_dmodel_t *)&sid_dmodel[dmodel];

  // vary gatelength depending on PAR1 / 4
  s32 gatelength = v->dm->gatelength;
  if( gatelength ) {
    gatelength += ((s32)voice_patch->D.par1 - 0x80) / 4;
    if( gatelength > 127 ) gatelength = 127; else if( gatelength < 2 ) gatelength = 2;
    // gatelength should never be 0, otherwise gate clear delay feature would be deactivated
  }
  v->drum_gatelength = gatelength;

  // vary WT speed depending on PAR2 / 4
  s32 wt_speed = v->dm->wt_speed;
  wt_speed += ((s32)voice_patch->D.par2 - 0x80) / 4;
  if( wt_speed > 127 ) wt_speed = 127; else if( wt_speed < 0 ) wt_speed = 0;
  v->drum_wt_speed = wt_speed;

  // store the third parameter
  v->drum_par3 = voice_patch->D.par3;

  // store waveform and note
  v->drum_waveform = v->dm->waveform;
  v->note = v->dm->base_note;

  // set/clear ACCENT depending on velocity
  v->state.ACCENT = velocity >= 64 ? 1 : 0;

  // activate voice and request gate
  v->state.VOICE_ACTIVE = 1;
  v->trg_dst[0] |= (1 << v->voice); // request gate
  v->trg_dst[2] |= (1 << v->voice); // reset wavetable

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Note Off Handler
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_D_NoteOff(u8 sid, u8 drum)
{
  if( drum >= 16 )
    return -1; // unsupported drum

  // go through all voices which are assigned to the current instrument
  sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[sid][0];
  int voice;
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
    if( v->assigned_instrument == drum ) {
      // release voice
      SID_VOICE_Release(sid, voice);

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SID_SE_D_NoteOff:%d] drum %d releases voice %d\n", sid, drum, voice);
#endif

      // the rest is only required if gatelength not controlled from drum model
      if( v->dm != NULL && v->drum_gatelength ) {
	v->state.VOICE_ACTIVE = 0;
	v->state.GATE_SET_REQ = 0;
	v->state.GATE_CLR_REQ = 1;
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// All Notes Off
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_D_AllNotesOff(u8 sid)
{
  sid_se_voice_t *v = (sid_se_voice_t *)&sid_se_voice[sid][0];
  int voice;

  // disable all active voices
  for(voice=0; voice<SID_SE_NUM_VOICES; ++voice, ++v) {
    if( v->state.VOICE_ACTIVE ) {
      // release voice
      SID_VOICE_Release(sid, voice);

#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[SID_SE_D_NoteOff:%d] drum %d releases voice %d\n", sid, drum, voice);
#endif

      // the rest is only required if gatelength not controlled from drum model
      if( !v->drum_gatelength ) {
	v->state.VOICE_ACTIVE = 0;
	v->state.GATE_SET_REQ = 0;
	v->state.GATE_CLR_REQ = 1;
      }
    }
  }

  return 0; // no error
}

