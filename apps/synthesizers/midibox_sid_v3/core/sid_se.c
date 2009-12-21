// $Id$
/*
 * MBSID Sound Engine Routines
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
#include "sid_se_l.h"
#include "sid_patch.h"
#include "sid_asid.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
// should be at least 1 for sending error messages
/////////////////////////////////////////////////////////////////////////////
#define DEBUG_VERBOSE_LEVEL 1


// measure performance with the stopwatch
#define STOPWATCH_PERFORMANCE_MEASURING 1


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

u8 sid_se_speed_factor;

sid_se_vars_t sid_se_vars[SID_NUM];
sid_se_voice_t sid_se_voice[SID_NUM][SID_SE_NUM_VOICES];
sid_se_filter_t sid_se_filter[SID_NUM][SID_SE_NUM_FILTERS];
sid_se_midi_voice_t sid_se_midi_voice[SID_NUM][SID_SE_NUM_MIDI_VOICES];
sid_se_lfo_t sid_se_lfo[SID_NUM][SID_SE_NUM_LFO];
sid_se_env_t sid_se_env[SID_NUM][SID_SE_NUM_ENV];
sid_se_wt_t sid_se_wt[SID_NUM][SID_SE_NUM_WT];
sid_se_seq_t sid_se_seq[SID_NUM];


#include "sid_se_frq_table.inc"
#include "sid_se_env_table.inc"
#include "sid_se_lfo_table.inc"
#include "sid_se_sin_table.inc"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_Init(u32 mode)
{
  int i, sid;
  s32 status = 0;

  // currently the sound engine runs two times faster than MBSID V2 (will be changed in future)
  sid_se_speed_factor = 2;

  for(sid=0; sid<SID_NUM; ++sid) {
    SID_SE_VarsInit((sid_se_vars_t *)&sid_se_vars[sid]);

    for(i=0; i<SID_SE_NUM_VOICES; ++i)
      SID_SE_VoiceInit((sid_se_voice_t *)&sid_se_voice[sid][i], sid, i);

    for(i=0; i<SID_SE_NUM_FILTERS; ++i)
      SID_SE_FilterInit((sid_se_filter_t *)&sid_se_filter[sid][i], sid, i);

    for(i=0; i<SID_SE_NUM_MIDI_VOICES; ++i)
      SID_SE_MIDIVoiceInit((sid_se_midi_voice_t *)&sid_se_midi_voice[sid][i]);

    for(i=0; i<SID_SE_NUM_LFO; ++i)
      SID_SE_LFOInit((sid_se_lfo_t *)&sid_se_lfo[sid][i], sid, i);

    for(i=0; i<SID_SE_NUM_ENV; ++i)
      SID_SE_ENVInit((sid_se_env_t *)&sid_se_env[sid][i], sid, i);

    for(i=0; i<SID_SE_NUM_WT; ++i)
      SID_SE_WTInit((sid_se_wt_t *)&sid_se_wt[sid][i], sid, i);

    SID_SE_SEQInit((sid_se_seq_t *)&sid_se_seq[sid]);
  }

  // initialize engine subfunctions
  status |= SID_SE_L_Init(mode);

  // start timer
  // TODO: increase sid_se_speed_factor once performance has been evaluated
  MIOS32_TIMER_Init(2, 2000 / sid_se_speed_factor, SID_SE_Update, MIOS32_IRQ_PRIO_MID);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialises sound engine variables
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_VarsInit(sid_se_vars_t *vars)
{
  // clear complete structure
  memset(vars, 0, sizeof(sid_se_vars_t));

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises a voice
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_VoiceInit(sid_se_voice_t *v, u8 sid, u8 voice)
{
  // clear complete structure
  memset(v, 0, sizeof(sid_se_voice_t));

  v->sid = sid; // cross-reference to assigned SID
  v->phys_sid = 2*sid + ((voice>=3) ? 1 : 0); // reference to physical SID (chip)
  v->voice = voice; // cross-reference to assigned voice
  v->phys_voice = voice % 3; // reference to physical voice
  v->phys_sid_voice = (sid_voice_t *)&sid_regs[v->phys_sid].v[v->phys_voice];
  v->voice_patch = (sid_se_voice_patch_t *)&sid_patch[sid].L.voice[voice];
  v->midi_voice = (sid_se_midi_voice_t *)&sid_se_midi_voice[sid][voice];
  v->notestack = (notestack_t *)&v->midi_voice->notestack;
  v->mod_dst_pitch = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_PITCH1 + voice];
  v->mod_dst_pw = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_PW1 + voice];
  v->trg_mask_note_on = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_NOn][0];
  v->trg_mask_note_off = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_NOff][0];
  v->trg_dst = (u8 *)&sid_se_vars[sid];

  v->pitchbender = 0x80;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises a filter
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_FilterInit(sid_se_filter_t *f, u8 sid, u8 filter)
{
  // clear complete structure
  memset(f, 0, sizeof(sid_se_filter_t));

  f->sid = sid; // cross-reference to assigned SID
  f->phys_sid = 2*sid + filter; // reference to physical SID (chip)
  f->filter = filter; // cross-reference to assigned filter
  f->phys_sid_regs = (sid_regs_t *)&sid_regs[f->phys_sid];
  f->filter_patch = (sid_se_filter_patch_t *)&sid_patch[sid].L.filter[filter];
  f->mod_dst_filter = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_FIL1 + filter];

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialises a MIDI Voice
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_MIDIVoiceInit(sid_se_midi_voice_t *midi_voice)
{
  // clear complete structure
  memset(midi_voice, 0, sizeof(sid_se_midi_voice_t));

  NOTESTACK_Init(&midi_voice->notestack,
		 NOTESTACK_MODE_PUSH_TOP,
		 &midi_voice->notestack_items[0],
		 SID_SE_NOTESTACK_SIZE);

  midi_voice->split_upper = 0x7f;
  midi_voice->transpose = 0x40;
  midi_voice->pitchbender = 0x80;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises a LFO
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_LFOInit(sid_se_lfo_t *l, u8 sid, u8 lfo)
{
  // clear complete structure
  memset(l, 0, sizeof(sid_se_lfo_t));

  l->sid = sid; // cross-reference to assigned SID
  l->lfo = lfo; // cross-reference to LFO number
  l->lfo_patch = (sid_se_lfo_patch_t *)&sid_patch[sid].L.lfo[lfo];
  l->mod_src_lfo = &sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_LFO1 + lfo];
  l->mod_dst_lfo_depth = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_LD1 + lfo];
  l->mod_dst_lfo_rate = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_LR1 + lfo];
  l->trg_mask_lfo_period = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_L1P + lfo][0];
  l->trg_dst = (u8 *)&sid_se_vars[sid];

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises an Envelope
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_ENVInit(sid_se_env_t *e, u8 sid, u8 env)
{
  // just clear complete structure
  memset(e, 0, sizeof(sid_se_env_t));

  e->sid = sid; // cross-reference to assigned SID
  e->env = env; // cross-reference to ENV number
  e->env_patch = (sid_se_env_patch_t *)&sid_patch[sid].L.env[env];
  e->mod_src_env = &sid_se_vars[sid].mod_src[SID_SE_MOD_SRC_ENV1 + env];
  e->trg_mask_env_sustain = (u8 *)&sid_patch[sid].L.trg_matrix[SID_SE_TRG_E1S + env][0];
  e->trg_dst = (u8 *)&sid_se_vars[sid];

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises a Wavetable
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_WTInit(sid_se_wt_t *w, u8 sid, u8 wt)
{
  // just clear complete structure
  memset(w, 0, sizeof(sid_se_wt_t));

  w->sid = sid; // cross-reference to assigned SID
  w->wt = wt; // cross-reference to WT number
  w->wt_patch = (sid_se_wt_patch_t *)&sid_patch[sid].L.wt[wt];
  w->mod_dst_wt = &sid_se_vars[sid].mod_dst[SID_SE_MOD_DST_WT1 + wt];

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initialises a Sequencer
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_SEQInit(sid_se_seq_t *seq)
{
  // just clear complete structure
  memset(seq, 0, sizeof(sid_se_seq_t));
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sound Engine Update Cycle
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_Update(void)
{
  // exit in ASID mode
  if( SID_ASID_ModeGet() != SID_ASID_MODE_OFF )
    return 0;

  // TODO: support for multiple SIDs
  u8 sid = 0;
  sid_se_engine_t engine = sid_patch[sid].engine;

#if STOPWATCH_PERFORMANCE_MEASURING >= 1
  APP_StopwatchReset();
#endif

  switch( engine ) {
    case SID_SE_LEAD: SID_SE_L_Update(sid); break;
  }

#if STOPWATCH_PERFORMANCE_MEASURING == 1
  APP_StopwatchCapture();
#endif

  // update SID registers
  SID_Update(0);

#if STOPWATCH_PERFORMANCE_MEASURING == 2
  APP_StopwatchCapture();
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Shared Help Functions
/////////////////////////////////////////////////////////////////////////////
s32 SID_SE_Gate(sid_se_voice_t *v)
{
  s32 change_pitch = 1;

  // voice disable handling (allows to turn on/off voice via waveform parameter)
  sid_se_voice_waveform_t waveform = (sid_se_voice_waveform_t)v->voice_patch->waveform;
  if( v->state.VOICE_DISABLED ) {
    if( !waveform.VOICE_OFF ) {
      v->state.VOICE_DISABLED = 0;
      if( v->state.VOICE_ACTIVE )
	v->state.GATE_SET_REQ = 1;
    }
  } else {
    if( waveform.VOICE_OFF ) {
      v->state.VOICE_DISABLED = 1;
      v->state.GATE_CLR_REQ = 1;
    }
  }

  // if gate not active: ignore clear request
  if( !v->state.GATE_ACTIVE )
    v->state.GATE_CLR_REQ = 0;

  // gate set/clear request?
  if( v->state.GATE_CLR_REQ ) {
    v->state.GATE_CLR_REQ = 0;

    // test bit allows to sync an oscillator
    v->phys_sid_voice->test = 0;

    // clear SID gate flag if GSA (gate stays active) function not enabled
    sid_se_voice_flags_t voice_flags = (sid_se_voice_flags_t)v->voice_patch->flags;
    if( !voice_flags.GSA )
      v->phys_sid_voice->gate = 0;

    // gate not active anymore
    v->state.GATE_ACTIVE = 0;

    // sync with SID register update (cleared by SID Register Update handler)
    sid_gate_update_done[v->sid] &= ~(1 << v->voice);
  } else if( v->state.GATE_SET_REQ ) {
    // skip so long voice hasn't been updated
    if( (sid_gate_update_done[v->sid] & (1 << v->voice)) ) {
      // don't set gate if oscillator disabled
      if( !waveform.VOICE_OFF ) {
	sid_se_opt_flags_t opt_flags = (sid_se_opt_flags_t)sid_patch[v->sid].opt_flags;

	// delay note so long 16bit delay counter != 0
	if( v->set_delay_ctr ) {
	  int delay_inc = v->voice_patch->delay;
	  // if ABW (ADSR bug workaround) active: use at least 30 mS delay
	  if( opt_flags.ABW ) {
	    delay_inc += 25;
	  }

	  // TODO: get delay counter from env

	  change_pitch = 0; // don't change pitch so long delay is active
	} else {
	  // now acknowledge the set request
	  v->state.GATE_SET_REQ = 0;

	  // if ABW (ADSR bug workaround) function active: update ADSR registers now!
	  if( opt_flags.ABW ) {
	    v->phys_sid_voice->ad = v->voice_patch->ad;
	    v->phys_sid_voice->sr = v->voice_patch->sr;
	  }

	  // TODO: OSC Sync

	  // set gate
	  v->phys_sid_voice->gate = 1;
	}
      }
      v->state.GATE_ACTIVE = 1;
    }
  }

  return change_pitch;
}

s32 SID_SE_Pitch(sid_se_voice_t *v)
{
  // transpose MIDI note
  sid_se_voice_arp_mode_t arp_mode = (sid_se_voice_arp_mode_t)v->voice_patch->arp_mode;
  int transposed_note = arp_mode.ENABLE ? v->arp_note : v->note;

  transposed_note += (int)v->voice_patch->transpose - 64;
  transposed_note += (int)v->midi_voice->transpose - 64;

  // octave wise saturation
  if( transposed_note < 0 ) {
    while( transposed_note < 0 )
      transposed_note += 12;
  } else if( transposed_note > 127 ) {
    while( transposed_note > 127 )
      transposed_note -= 12;
  }

  // transfer note -> linear frequency
  int target_frq = transposed_note << 9;

  // TODO: Glissando
  sid_se_voice_flags_t voice_flags = (sid_se_voice_flags_t)v->voice_patch->flags;


  // increase/decrease target frequency by pitchrange
  // depending on pitchbender and finetune value
  if( v->voice_patch->pitchrange ) {
    u16 pitchbender = v->pitchbender; // TODO: multi/bassline/drum engine take pitchbender from MIDI Voice
    int delta = (int)pitchbender - 0x80;
    delta += (int)v->voice_patch->finetune-0x80;

    // detuning
    u8 detune = sid_patch[v->sid].L.osc_detune;
    if( detune ) {
      // additional detuning depending on SID channel and oscillator
      // Left OSC1: +detune/4   (lead only, 0 in bassline mode)
      // Right OSC1: -detune/4  (lead only, 0 in bassline mode)
      // Left OSC2: +detune
      // Right OSC2: -detune
      // Left OSC3: -detune
      // Right OSC3: +detune
      switch( v->voice ) {
        case 0: delta += detune/4; break;
        case 3: delta -= detune/4; break;

        case 1:
        case 5: delta += detune; break;

        case 2:
        case 4: delta -= detune; break;
      }
    }

    if( delta ) {
      int scaled = (delta * 4 * (int)v->voice_patch->pitchrange);
      target_frq += scaled;
    }
  }

  // pitch modulation
  target_frq += *v->mod_dst_pitch;
  if( target_frq < 0 ) target_frq = 0; else if( target_frq > 0xffff ) target_frq = 0xffff;

  // portamento
  // whenever target frequency has been changed, update portamento frequency
  if( v->prev_target_frq != target_frq ) {
    v->prev_target_frq = target_frq;
    v->prev_linear_frq = v->linear_frq;
    // reset portamento counter if not in glissando mode
    if( voice_flags.PORTA_MODE < 2 )
      v->portamento_ctr = 0;

    if( target_frq == v->linear_frq )
      v->state.PORTA_ACTIVE = 0; // nothing to do...
  }

  int linear_frq = target_frq;
  u8 portamento;
  if( v->state.PORTA_ACTIVE && (portamento=v->voice_patch->portamento) && voice_flags.PORTA_MODE < 2 ) {
    linear_frq = v->linear_frq;

    // get portamento multiplier from envelope table
    // this one is used for "constant time glide" and "normal portamento"
    int porta_multiplier = sid_se_env_table[portamento] / sid_se_speed_factor;

    if( voice_flags.PORTA_MODE == 1 ) { // constant glide time
      // increment counter
      int portamento_ctr = v->portamento_ctr + porta_multiplier;

      // target reached on overrun
      if( portamento_ctr >= 0xffff ) {
	linear_frq = target_frq;
	v->state.PORTA_ACTIVE = 0;
      } else {
	v->portamento_ctr = portamento_ctr;

	// scale between new and old frequency
	int delta = v->prev_target_frq - v->prev_linear_frq;
	linear_frq = v->prev_linear_frq + ((delta * portamento_ctr) >> 16);
	if( delta > 0 ) {
	  if( linear_frq >= target_frq ) {
	    linear_frq = target_frq;
	    v->state.PORTA_ACTIVE = 0;
	  }
	} else {
	  if( linear_frq <= target_frq ) {
	    linear_frq = target_frq;
	    v->state.PORTA_ACTIVE = 0;
	  }
	}
      }

    } else { // normal portamento

      // increment/decrement frequency
      int inc = (int)(((u32)v->linear_frq * (u32)porta_multiplier) >> 16);
      if( target_frq > linear_frq ) {
	linear_frq += inc;
	if( linear_frq >= target_frq ) {
	  linear_frq = target_frq;
	  v->state.PORTA_ACTIVE = 0;
	}
      } else {
	linear_frq -= inc;
	if( linear_frq <= target_frq ) {
	  linear_frq = target_frq;
	  v->state.PORTA_ACTIVE = 0;
	}
      }
    }
  }

  // if frequency has been changed
  if( v->linear_frq != linear_frq ) {
    v->linear_frq = linear_frq;

    // linear frequency -> SID frequency conversion
    // convert bitfield [15:9] of 16bit linear frequency to SID frequency value
    u8 frq_ix = (linear_frq >> 9) + 21;
    if( frq_ix > 126 )
      frq_ix = 126;
    // interpolate between two frequency steps (frq_a and frq_b)
    int frq_a = sid_se_frq_table[frq_ix];
    int frq_b = sid_se_frq_table[frq_ix+1];

    // use bitfield [8:0] of 16bit linear frequency for scaling between two semitones
    int frq_semi = (frq_b-frq_a) * (linear_frq & 0x1ff);
    int frq = frq_a + (frq_semi >> 9);

    // write result into SID frequency register
    v->phys_sid_voice->frq_l = frq & 0xff;
    v->phys_sid_voice->frq_h = frq >> 8;
  }
  
  return 0; // no error
}

s32 SID_SE_PW(sid_se_voice_t *v)
{
  // convert pulsewidth to 12bit signed value
  int pulsewidth = ((v->voice_patch->pulsewidth_h & 0x0f) << 8) | v->voice_patch->pulsewidth_l;

  // PW modulation
  pulsewidth += *v->mod_dst_pw / 16;
  if( pulsewidth > 0xfff ) pulsewidth = 0xfff; else if( pulsewidth < 0 ) pulsewidth = 0;

  // transfer to SID registers
  v->phys_sid_voice->pw_l = pulsewidth & 0xff;
  v->phys_sid_voice->pw_h = (pulsewidth >> 8) & 0x0f;

  return 0; // no error
}


s32 SID_SE_FilterAndVolume(sid_se_filter_t *f)
{
  int cutoff = ((f->filter_patch->cutoff_h & 0x0f) << 8) | f->filter_patch->cutoff_l;

  // cutoff modulation
  cutoff += *f->mod_dst_filter / 16;
  if( cutoff > 0xfff ) cutoff = 0xfff; else if( cutoff < 0 ) cutoff = 0;

  // TODO: calibration

  // map 12bit value to 11 value of SID register
  f->phys_sid_regs->filter_l = (cutoff >> 1) & 0x7;
  f->phys_sid_regs->filter_h = (cutoff >> 4);

  // resonance (4bit only)
  int resonance = f->filter_patch->resonance;
  f->phys_sid_regs->resonance = resonance >> 4;

  // filter channel/mode selection
  u8 chn_mode = f->filter_patch->chn_mode;
  f->phys_sid_regs->filter_select = chn_mode & 0xf;
  f->phys_sid_regs->filter_mode = chn_mode >> 4;

  // volume
  int volume = sid_patch[f->sid].L.volume << 9;

  // TODO: modulation

  f->phys_sid_regs->volume = volume >> 12;

  return 0; // no error
}
