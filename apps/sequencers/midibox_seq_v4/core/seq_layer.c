// $Id$
/*
 * Sequencer Parameter Layer Routines
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
#include "tasks.h"

#include "seq_bpm.h"
#include "seq_layer.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_chord.h"
#include "seq_record.h"
#include "seq_live.h"
#include "seq_ui.h"
#include "seq_morph.h"
#include "seq_random.h"


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// to display activity of selected track in trigger/parameter selection page
u8 seq_layer_vu_meter[16];

#ifdef MBSEQV4P
// Drum CC configuration
u8 seq_layer_drum_cc[16][4]; // up to 4 parameter layers per drum
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions and arrays
/////////////////////////////////////////////////////////////////////////////

// preset table - each entry contains the initial track configuration and the step settings
// EVNT0 and CHN won't be overwritten
// for four steps (they are copied to all other 4step groups)
// structure of track settings matches with the SEQ_TRKRECORD in app_defines.h

// parameters which will be reset to the given values in all event modes
static const u8 seq_layer_preset_table_static[][2] = {
  // parameter             value
  { SEQ_CC_MODE,           SEQ_CORE_TRKMODE_Normal },
  { SEQ_CC_MODE_FLAGS,     0x03 },
  { SEQ_CC_BUSASG,         0x00 },
  { SEQ_CC_DIRECTION,	   0x00 },
  { SEQ_CC_STEPS_REPLAY,   0x00 },
  { SEQ_CC_STEPS_FORWARD,  0x00 },
  { SEQ_CC_STEPS_JMPBCK,   0x00 },
  { SEQ_CC_STEPS_REPEAT,   0x00 },
  { SEQ_CC_STEPS_SKIP,     0x00 },
  { SEQ_CC_STEPS_RS_INTERVAL,0x03 },
  { SEQ_CC_CLK_DIVIDER,    0x0f },
  { SEQ_CC_CLKDIV_FLAGS,   0x00 },
  { SEQ_CC_LENGTH,         0x0f },
  { SEQ_CC_LOOP,           0x00 },
  { SEQ_CC_TRANSPOSE_SEMI, 0x00 },
  { SEQ_CC_TRANSPOSE_OCT,  0x00 },
  { SEQ_CC_GROOVE_VALUE,   0x00 },
  { SEQ_CC_GROOVE_STYLE,   0x00 },
  { SEQ_CC_MORPH_MODE,     0x00 },
  { SEQ_CC_MORPH_DST,      0x00 },
  { SEQ_CC_HUMANIZE_VALUE, 0x00 },
  { SEQ_CC_HUMANIZE_MODE,  0x00 },
  { SEQ_CC_ECHO_REPEATS,   0x00 },
  { SEQ_CC_ECHO_DELAY,     0x07 }, // 1/8
  { SEQ_CC_ECHO_VELOCITY,  15 }, // 75%
  { SEQ_CC_ECHO_FB_VELOCITY, 15 }, // 75%
  { SEQ_CC_ECHO_FB_NOTE,   24 }, // +0
  { SEQ_CC_ECHO_FB_GATELENGTH, 20 }, // 100%
  { SEQ_CC_ECHO_FB_TICKS,  20 }, // 100%
  { SEQ_CC_LFO_WAVEFORM,   0x00 }, // off
  { SEQ_CC_LFO_AMPLITUDE,  128 + 64 },
  { SEQ_CC_LFO_PHASE,      0 },
  { SEQ_CC_LFO_STEPS,      15 },
  { SEQ_CC_LFO_STEPS_RST,  15 },
  { SEQ_CC_LFO_ENABLE_FLAGS, 0 },
  { SEQ_CC_LFO_CC,         0 },
  { SEQ_CC_LFO_CC_OFFSET,  64 },
  { SEQ_CC_LFO_CC_PPQN,    6 }, // 96 ppqn
  { SEQ_CC_LIMIT_LOWER,    0 },
  { SEQ_CC_LIMIT_UPPER,    0 },
  { 0xff,                  0xff } // end marker
};


// initial drum notes (can be customized in /PRESETS/TRKDRUMS.V4P)
static u8 seq_layer_preset_table_drum_notes[16] = {
  0x24, // BD
  0x26, // SD
  0x2a, // CH
  0x2c, // PH
  0x2e, // OH
  0x46, // MA
  0x27, // CP
  0x37, // CY
  0x29, // LT
  0x2b, // MT
  0x2f, // HT
  0x4b, // RS
  0x38, // CB
  0x2c, // Smp1
  0x3c, // Smp2
  0x3d  // Smp3
};


/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
static u8 pb_last_value[SEQ_CORE_NUM_TRACKS];
static u8 pc_last_value[SEQ_CORE_NUM_TRACKS];
static u8 at_last_value[SEQ_CORE_NUM_TRACKS];
#ifdef MBSEQV4P
static u8 cc_last_value[SEQ_CORE_NUM_TRACKS][16][4]; // up to 4 parameter layers per drum
#else
static u8 cc_last_value[SEQ_CORE_NUM_TRACKS][1][16];
#endif

static u8 track_pc_last_value[SEQ_CORE_NUM_TRACKS];
static u8 track_bank_h_last_value[SEQ_CORE_NUM_TRACKS];
static u8 track_bank_l_last_value[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_Init(u32 mode)
{
  // initialize parameter/trigger layers and CCs
  SEQ_PAR_Init(0);
  SEQ_TRG_Init(0);
  SEQ_CC_Init(0);

  SEQ_LAYER_ResetLatchedValues();
  SEQ_LAYER_ResetTrackPCBankLatchedValues();

  // copy preset into all tracks
  u8 track;
  for(track=0; track<16; ++track) {
    u8 only_layers = 0;
#ifndef MBSEQV4L
    u8 all_triggers_cleared = (track >= 1) ? 1 : 0; // triggers only set for first track
#else
    u8 all_triggers_cleared = 0; // trigger handling for all tracks
#endif
    u8 init_assignments = 1;
    SEQ_LAYER_CopyPreset(track, only_layers, all_triggers_cleared, init_assignments);
  }

#ifdef MBSEQV4P
  // initial Drum CC assignments
  u8 drum;
  for(drum=0; drum<16; ++drum) {
    u8 par_layer;
    for(par_layer=0; par_layer<4; ++par_layer) {
      seq_layer_drum_cc[drum][par_layer] = 16 + par_layer*16 + drum;
    }
  }
#endif

  SEQ_CHORD_Init(0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function clears the latched pitchbender, program change and CC values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_ResetLatchedValues(void)
{
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    pb_last_value[track] = 0xff; // invalid value - PB value will be send in any case
    pc_last_value[track] = 0xff; // invalid value - PC value will be send in any case
    at_last_value[track] = 0xff; // invalid value - AT value will be send in any case

    memset(cc_last_value, 0xff, sizeof(cc_last_value)); // invalid value - CC value will be send in any case
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function clears the latched track based program/bank change values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_ResetTrackPCBankLatchedValues(void)
{
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
    // invalid value - value will be send in any case
    track_pc_last_value[track] = 0xff;
    track_bank_h_last_value[track] = 0xff;
    track_bank_l_last_value[track] = 0xff;
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function sends the track based program/bank change values
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_SendPCBankValues(u8 track, u8 force, u8 send_now)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  mios32_midi_port_t port = tcc->midi_port;
  mios32_midi_chn_t chn = tcc->midi_chn;

  {
    if( force || (tcc->midi_bank_l && track_bank_l_last_value[track] != (tcc->midi_bank_l-1)) )
      track_bank_l_last_value[track] = tcc->midi_bank_l;
    if( send_now && tcc->midi_bank_l )
      MIOS32_MIDI_SendCC(port, chn, 0x20, tcc->midi_bank_l - 1);
  }

  {
    if( force || (tcc->midi_bank_h && track_bank_h_last_value[track] != (tcc->midi_bank_h-1)) )
      track_bank_h_last_value[track] = tcc->midi_bank_h - 1;
    if( send_now && tcc->midi_bank_h )
      MIOS32_MIDI_SendCC(port, chn, 0x00, tcc->midi_bank_h - 1);
  }

  {
    if( force || (tcc->midi_pc && track_pc_last_value[track] != (tcc->midi_pc-1)) )
      track_pc_last_value[track] = tcc->midi_pc - 1;
    if( send_now && tcc->midi_pc )
      MIOS32_MIDI_SendProgramChange(port, chn, tcc->midi_pc - 1);
  }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This function allows to change the default drum note
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_PresetDrumNoteSet(u8 num, u8 note)
{
  if( num >= 16 ) {
    return -1; // invalid note
  }

  seq_layer_preset_table_drum_notes[num] = note;

  return 0; // no error
}


s32 SEQ_LAYER_PresetDrumNoteGet(u8 num)
{
  if( num >= 16 ) {
    return -1; // invalid note
  }

  return seq_layer_preset_table_drum_notes[num];
}


/////////////////////////////////////////////////////////////////////////////
// This function returns a string to the event mode name (5 chars)
/////////////////////////////////////////////////////////////////////////////
// Note: newer gcc versions don't allow to return a "const" parameter, therefore
// this array is declared outside the SEQ_LAYER_GetEvntModeName() function
static const char event_mode_str[5+1][7] = { "Note ", "Chord", " CC  ", "Drum ", "Comb ", "?????" };

const char *SEQ_LAYER_GetEvntModeName(seq_event_mode_t event_mode)
{
  if( event_mode < 5 )
    return event_mode_str[event_mode];
  else
    return event_mode_str[5];
}


/////////////////////////////////////////////////////////////////////////////
// This function returns the layer_evnt_t information based on given
// layer (used for visualisation purposes)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_GetEvntOfLayer(u8 track, u16 step, u8 layer, u8 instrument, seq_layer_evnt_t *layer_event)
{
  s32 number_of_events = 0;

  seq_cc_trk_t *tcc = &seq_cc_trk[track];
#ifdef MBSEQV4P
  seq_layer_evnt_t layer_events[83];
  number_of_events = SEQ_LAYER_GetEventsPlus(track, step, layer_events, 1);
#else
  seq_layer_evnt_t layer_events[16];
  number_of_events = SEQ_LAYER_GetEvents(track, step, layer_events, 1);
#endif

  if( number_of_events <= 0 ) {
    // fill with dummy data
    layer_event->midi_package.ALL = 0;
    layer_event->len = -1;

    return -1; // no valid event
  }

  // search layer to which the event belongs to
  u8 event_num;
  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    for(event_num=0; event_num<number_of_events; ++event_num) {
      if( instrument == layer_events[event_num].layer_tag )
	break;
    }
  } else {
    for(event_num=0; event_num<number_of_events; ++event_num) {
      if( layer == layer_events[event_num].layer_tag )
	break;
    }
  }

  if( event_num >= number_of_events )
    event_num = 0;

  layer_event->midi_package = layer_events[event_num].midi_package;
  layer_event->len = layer_events[event_num].len;
  layer_event->layer_tag = layer;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns all events of a selected step
// Note: MBSEQV4P supports CCs on drum tracks, therefore we've to support
// +64 events which can be generated at once
// In order to ensure that layer_events array is prepared for this, we give
// the function a dedicated name for this case
/////////////////////////////////////////////////////////////////////////////
#ifdef MBSEQV4P
s32 SEQ_LAYER_GetEventsPlus(u8 track, u16 step, seq_layer_evnt_t layer_events[83], u8 insert_empty_notes)
#else
s32 SEQ_LAYER_GetEvents(u8 track, u16 step, seq_layer_evnt_t layer_events[16], u8 insert_empty_notes)
#endif
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  u16 layer_muted = seq_core_trk[track].layer_muted | seq_core_trk[track].layer_muted_from_midi;
  u8 num_events = 0;

  u8 handle_vu_meter = (ui_page == SEQ_UI_PAGE_TRGSEL || ui_page == SEQ_UI_PAGE_INSSEL || ui_page == SEQ_UI_PAGE_PARSEL || ui_page == SEQ_UI_PAGE_MUTE) && track == SEQ_UI_VisibleTrackGet();

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(track); // we assume, that PAR layer has same number of instruments!

    u8 drum;
    for(drum=0; drum<num_instruments; ++drum) {
      u8 note = tcc->lay_const[0*16 + drum];
      u8 velocity = 0;

      if( !insert_empty_notes && (layer_muted & (1 << drum)) )
	velocity = 0;
      else {
	if( SEQ_TRG_Get(track, step, 0, drum) ) {
	  if( tcc->link_par_layer_velocity >= 0 )
	    velocity = SEQ_PAR_VelocityGet(track, step, drum, 0x0000);
	  else
	    velocity = tcc->lay_const[1*16 + drum];
	}
      }

      if( !insert_empty_notes ) {
	u8 rnd_probability;
	if( (rnd_probability=SEQ_PAR_ProbabilityGet(track, step, drum, layer_muted)) < 100 &&
	    SEQ_RANDOM_Gen_Range(0, 99) >= rnd_probability )
	  velocity = 0;
      }

      if( handle_vu_meter && velocity ) {
	seq_layer_vu_meter[drum] = (velocity > 8) ? velocity : 1;
      } else {
	seq_layer_vu_meter[drum] &= 0x7f; // ensure that no static assignment is displayed
      }

      if( (note && velocity) || insert_empty_notes ) {
	seq_layer_evnt_t *e = &layer_events[num_events];
	mios32_midi_package_t *p = &e->midi_package;

	p->type     = NoteOn;
	p->cable    = track;
	p->event    = NoteOn;
	p->chn      = tcc->midi_chn; // TODO: optionally different channel taken from const D
	p->note     = note;
	p->velocity = velocity;
	e->len = SEQ_PAR_LengthGet(track, step, drum, 0x0000);
	e->layer_tag = drum;

	++num_events;
      }
    }

#ifdef MBSEQV4P
    // CC, PitchBend, etc only supported by V4+
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    u8 pb_sent = 0;
    u8 at_sent = 0;
    u8 pc_sent = 0;

    for(drum=0; drum<num_instruments; ++drum) {
      u8 *layer_type_ptr = (u8 *)&tcc->par_assignment_drum[0];
      int par_layer;
      for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {

	switch( *layer_type_ptr ) {
	case SEQ_PAR_Type_CC: {
	  u8 cc_number = seq_layer_drum_cc[drum][par_layer];
	  u8 value = SEQ_PAR_Get(track, step, par_layer, drum);

	  if( !insert_empty_notes ) {
	    // new: don't send CC if assigned to invalid CC number
	    // new: don't send if CC is assigned to LFO extra CC function
	    if( cc_number >= 0x80 ||
		(tcc->lfo_waveform && tcc->lfo_cc == cc_number) )
	      break;

	    // don't send CC if value hasn't changed (== invalid value)
	    // but only if LFO not assigned to CC layer
	    if( !tcc->lfo_enable_flags.CC &&
		( value >= 0x80 || value == cc_last_value[track][drum][par_layer]) ) {
	      break;
	    }
	    cc_last_value[track][drum][par_layer] = value;
	  }

	  if( insert_empty_notes || !(layer_muted & (1 << drum)) ) {
	    seq_layer_evnt_t *e = &layer_events[num_events];
	    mios32_midi_package_t *p = &e->midi_package;

	    p->type     = CC;
	    p->cable    = track;
	    p->event    = CC;
	    p->chn      = tcc->midi_chn;
	    p->cc_number = cc_number;
	    p->value    = value;
	    e->len      = -1;
	    e->layer_tag = drum;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventCC(track, step, e, drum, par_layer);
	  }
	} break;

	case SEQ_PAR_Type_Ctrl: {
	  u8 cc_number = seq_layer_drum_cc[drum][par_layer];
	  u8 value = SEQ_PAR_Get(track, step, par_layer, drum);

	  if( !insert_empty_notes ) {
	    // new: don't send CC if assigned to invalid CC number
	    if( cc_number >= 0x80 ) {
	      break;
	    }
	  }

	  if( insert_empty_notes || !(layer_muted & (1 << drum)) ) {
	    // Quick&Dirty: forward immediately to SEQ CC (TODO: we could do this in seq_core, maybe cleaner)
	    SEQ_CC_MIDI_Set(track, cc_number, value);
	  }
	} break;

        case SEQ_PAR_Type_PitchBend: {
	  if( pb_sent ) // send only once!
	    break;
	  pb_sent = 1;

	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, drum);

	  // don't send pitchbender if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == pb_last_value[track] )
	      break;
	    pb_last_value[track] = value;
	  }

	  if( insert_empty_notes || !(layer_muted & (1 << drum)) ) {
	    p->type     = PitchBend;
	    p->cable    = track;
	    p->event    = PitchBend;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = (value == 0x40) ? 0x00 : value; // LSB
	    p->evnt2    = value; // MSB
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventPitchBend(track, step, e, drum, par_layer);
	  }

	} break;

        case SEQ_PAR_Type_ProgramChange: {
	  if( pc_sent ) // send only once!
	    break;
	  pc_sent = 1;

	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, drum);

	  // don't send program change if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == pc_last_value[track] )
	      break;
	    pc_last_value[track] = value;
	  }

	  if( insert_empty_notes || !(layer_muted & (1 << drum)) ) {
	    p->type     = ProgramChange;
	    p->cable    = track;
	    p->event    = ProgramChange;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = value;
	    p->evnt2    = 0x00; // don't care
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventProgramChange(track, step, e, drum, par_layer);
	  }

	} break;

        case SEQ_PAR_Type_Aftertouch: {
	  if( at_sent ) // send only once!
	    break;
	  at_sent = 1;

	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, drum);

	  // don't send aftertouch if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == at_last_value[track] )
	      break;
	    at_last_value[track] = value;
	  }

	  if( insert_empty_notes || !(layer_muted & (1 << drum)) ) {
	    p->type     = Aftertouch;
	    p->cable    = track;
	    p->event    = Aftertouch;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = value;
	    p->evnt2    = 0x00; // don't care
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventAftertouch(track, step, e, drum, par_layer);
	  }

	} break;

	}

	if( num_events >= 83 )
	  break;
      }
    }
#endif

  } else {
    u8 instrument = 0;
    int par_layer;

    // get velocity and length from first parameter layer which holds it
    // if not assigned, we will get back a default value
    
    u8 gate = SEQ_TRG_GateGet(track, step, instrument);
    u8 velocity = 100; // default velocity
    if( tcc->event_mode != SEQ_EVENT_MODE_Combined ) {
      if( (par_layer=tcc->link_par_layer_velocity) >= 0 ) {
	if( insert_empty_notes || !(layer_muted & (1 << par_layer)) ) {
	  velocity = SEQ_PAR_Get(track, step, par_layer, instrument);
	  if( !insert_empty_notes && !gate )
	    velocity = 0;
	} else {
	  if( !gate )
	    velocity = 0;
	}

	if( handle_vu_meter )
	  seq_layer_vu_meter[par_layer] = ((velocity > 8) ? velocity : 8) | 0x80;
      } else {
	if( !insert_empty_notes && !gate )
	  velocity = 0;
      }
    }

    u8 length = 71; // default length
    if( tcc->event_mode != SEQ_EVENT_MODE_Combined ) {
      if( (par_layer=tcc->link_par_layer_length) >= 0 ) {
	if( (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	  length = SEQ_PAR_Get(track, step, par_layer, instrument) + 1;
	  if( length > 96 )
	    length = 96;
	}

	if( handle_vu_meter )
	  seq_layer_vu_meter[par_layer] = ((length > 8) ? length : 8) | 0x80;
      }
    }

    if( handle_vu_meter ) { // only for VU meters
      if( (par_layer=tcc->link_par_layer_probability) ) { // Probability
	u8 rnd_probability = SEQ_PAR_ProbabilityGet(track, step, instrument, layer_muted);
	seq_layer_vu_meter[par_layer] = ((rnd_probability > 8) ? rnd_probability : 8) | 0x80;
      }

      if( (par_layer=tcc->link_par_layer_delay) ) { // Delay
	u8 delay = SEQ_PAR_StepDelayGet(track, step, instrument, layer_muted);
	seq_layer_vu_meter[par_layer] = ((delay > 8) ? delay : 8) | 0x80;
      }

      if( (par_layer=tcc->link_par_layer_roll) ) { // Roll mode
	u8 roll_mode = SEQ_PAR_RollModeGet(track, step, instrument, layer_muted);
	if( roll_mode )
	  seq_layer_vu_meter[par_layer] = 0x7f;
      }

      if( (par_layer=tcc->link_par_layer_roll2) ) { // Roll2 mode
	u8 roll2_mode = SEQ_PAR_Roll2ModeGet(track, step, instrument, layer_muted);
	if( roll2_mode )
	  seq_layer_vu_meter[par_layer] = 0x7f;
      }
    }

    // go through all layers to generate events
    u8 *layer_type_ptr = (u8 *)&tcc->lay_const[0*16];
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);
    for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {

      // usually no function assigned to layer - skip it immediately to speed up the loop
      if( *layer_type_ptr == SEQ_PAR_Type_None )
	continue;

      // branch depending on layer type
      switch( *layer_type_ptr ) {

        case SEQ_PAR_Type_Note: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 note = SEQ_PAR_Get(track, step, par_layer, instrument);

	  if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	    if( (track&7) == 1 || (track&7) == 2)
	      return 0; // assigned to velocity and length
	  
	    if( !insert_empty_notes && !gate )
	      velocity = 0;
	    else
	      velocity = SEQ_PAR_Get(track+1, step, par_layer, instrument);

	    length = SEQ_PAR_Get(track+2, step, par_layer, instrument) + 1;
	    if( length > 96 )
	      length = 96;
	  }

	  if( !insert_empty_notes && (layer_muted & (1 << par_layer)) )
	    note = 0;

	  if( note || insert_empty_notes ) {
	    p->type     = NoteOn;
	    p->cable    = track;
	    p->event    = NoteOn;
	    p->chn      = tcc->midi_chn;
	    p->note     = note;
	    p->velocity = velocity;
	    e->len      = length;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && velocity && tcc->morph_mode )
	      SEQ_MORPH_EventNote(track, step, e, instrument, par_layer, tcc->link_par_layer_velocity, tcc->link_par_layer_length);
	  }

	  if( handle_vu_meter && note && velocity )
	    seq_layer_vu_meter[par_layer] = ((velocity > 8) ? velocity : 8);
	} break;

        case SEQ_PAR_Type_Chord1:
        case SEQ_PAR_Type_Chord2:
        case SEQ_PAR_Type_Chord3: {
	  u8 chord_value = SEQ_PAR_Get(track, step, par_layer, instrument);
	  int i;

	  if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	    if( (track&7) == 1 || (track&7) == 2)
	      return 0; // assigned to velocity and length
	  
	    if( !insert_empty_notes && !gate )
	      velocity = 0;
	    else
	      velocity = SEQ_PAR_Get(track+1, step, par_layer, instrument);

	    length = SEQ_PAR_Get(track+2, step, par_layer, instrument) + 1;
	    if( length > 96 )
	      length = 96;
	  }

	  if( chord_value || insert_empty_notes ) {
	    seq_layer_evnt_t e_proto;
	    mios32_midi_package_t *p_proto = &e_proto.midi_package;
	    p_proto->type     = NoteOn;
	    p_proto->cable    = track;
	    p_proto->event    = NoteOn;
	    p_proto->chn      = tcc->midi_chn;
	    p_proto->note     = chord_value; // we will determine the note value later
	    p_proto->velocity = velocity;
	    e_proto.len      = length;
	    e_proto.layer_tag = par_layer;

	    // morph chord value like a note (-> nice effect!)
	    if( !insert_empty_notes && velocity && tcc->morph_mode ) {
	      SEQ_MORPH_EventNote(track, step, &e_proto, instrument, par_layer, tcc->link_par_layer_velocity, tcc->link_par_layer_length);
	      chord_value = e_proto.midi_package.note; // chord value has been morphed!
	    }

	    for(i=0; i<6; ++i) {
	      if( num_events >= 16 )
		break;

	      u8 chord_set = (*layer_type_ptr == SEQ_PAR_Type_Chord2) ? 1 : (((*layer_type_ptr == SEQ_PAR_Type_Chord3) ? 2 : 0));
	      s32 note = SEQ_CHORD_NoteGet(i, chord_set, chord_value);

	      if( !insert_empty_notes && (layer_muted & (1 << par_layer)) )
		note = 0;

	      if( note < 0 )
		break;

	      seq_layer_evnt_t *e = &layer_events[num_events];
	      *e = e_proto;
	      e->midi_package.note = note;
	      ++num_events;
	    }
	  }

	  if( handle_vu_meter && velocity )
	    seq_layer_vu_meter[par_layer] = ((velocity > 8) ? velocity : 8);

	} break;

        case SEQ_PAR_Type_CC: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 cc_number = tcc->lay_const[1*16 + par_layer];
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  if( !insert_empty_notes ) {
	    // new: don't send CC if assigned to invalid CC number
	    // new: don't send if CC is assigned to LFO extra CC function
	    if( cc_number >= 0x80 ||
		(tcc->lfo_waveform && tcc->lfo_cc == cc_number) )
	      break;

	    // don't send CC if value hasn't changed (== invalid value)
	    // but only if LFO not assigned to CC layer
	    if( !tcc->lfo_enable_flags.CC &&
		( value >= 0x80 || value == cc_last_value[track][0][par_layer]) ) {
	      break;
	    }
	    cc_last_value[track][0][par_layer] = value;
	  }

	  if(
#ifndef MBSEQV4L
	     (tcc->event_mode != SEQ_EVENT_MODE_CC || gate) &&
#endif
	     (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	    p->type     = CC;
	    p->cable    = track;
	    p->event    = CC;
	    p->chn      = tcc->midi_chn;
	    p->cc_number = cc_number;
	    p->value    = value;
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventCC(track, step, e, instrument, par_layer);

	    if( handle_vu_meter )
	      seq_layer_vu_meter[par_layer] = ((p->value > 8) ? p->value : 8) | 0x80;
	  }

	} break;

        case SEQ_PAR_Type_Ctrl: {
	  u8 cc_number = tcc->lay_const[1*16 + par_layer];
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  if( !insert_empty_notes ) {
	    // new: don't send CC if assigned to invalid CC number
	    if( cc_number >= 0x80 ) {
	      break;
	    }
	  }

	  if(
#ifndef MBSEQV4L
	     (tcc->event_mode != SEQ_EVENT_MODE_CC || gate) &&
#endif
	     (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	    // Quick&Dirty: forward immediately to SEQ CC (TODO: we could do this in seq_core, maybe cleaner)
	    SEQ_CC_MIDI_Set(track, cc_number, value);
	    
	    if( handle_vu_meter )
	      seq_layer_vu_meter[par_layer] = ((value > 8) ? value : 8) | 0x80;
	  }

	} break;

        case SEQ_PAR_Type_PitchBend: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  // don't send pitchbender if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == pb_last_value[track] )
	      break;
	    pb_last_value[track] = value;
	  }

	  if(
#ifndef MBSEQV4L
	     (tcc->event_mode != SEQ_EVENT_MODE_CC || gate) &&
#endif
	     (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	    p->type     = PitchBend;
	    p->cable    = track;
	    p->event    = PitchBend;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = (value == 0x40) ? 0x00 : value; // LSB
	    p->evnt2    = value; // MSB
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventPitchBend(track, step, e, instrument, par_layer);

	    if( handle_vu_meter )
	      seq_layer_vu_meter[par_layer] = ((p->evnt2 > 8) ? p->evnt2 : 8) | 0x80;
	  }

	} break;

        case SEQ_PAR_Type_ProgramChange: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  // don't send program change if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == pc_last_value[track] )
	      break;
	    pc_last_value[track] = value;
	  }

	  if( (tcc->event_mode != SEQ_EVENT_MODE_CC || gate) &&
	      (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	    p->type     = ProgramChange;
	    p->cable    = track;
	    p->event    = ProgramChange;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = value;
	    p->evnt2    = 0x00; // don't care
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventProgramChange(track, step, e, instrument, par_layer);

	    if( handle_vu_meter )
	      seq_layer_vu_meter[par_layer] = ((p->evnt1 > 8) ? p->evnt1 : 8) | 0x80;
	  }

	} break;

        case SEQ_PAR_Type_Aftertouch: {
	  seq_layer_evnt_t *e = &layer_events[num_events];
	  mios32_midi_package_t *p = &e->midi_package;
	  u8 value = SEQ_PAR_Get(track, step, par_layer, instrument);

	  // don't send aftertouch if value hasn't changed
	  if( !insert_empty_notes ) {
	    if( value >= 0x80 || value == at_last_value[track] )
	      break;
	    at_last_value[track] = value;
	  }

	  if( (tcc->event_mode != SEQ_EVENT_MODE_CC || gate) &&
	      (insert_empty_notes || !(layer_muted & (1 << par_layer))) ) {
	    p->type     = Aftertouch;
	    p->cable    = track;
	    p->event    = Aftertouch;
	    p->chn      = tcc->midi_chn;
	    p->evnt1    = value;
	    p->evnt2    = 0x00; // don't care
	    e->len      = -1;
	    e->layer_tag = par_layer;
	    ++num_events;

	    // morph it
	    if( !insert_empty_notes && tcc->morph_mode )
	      SEQ_MORPH_EventAftertouch(track, step, e, instrument, par_layer);

	    if( handle_vu_meter )
	      seq_layer_vu_meter[par_layer] = ((p->evnt1 > 8) ? p->evnt1 : 8) | 0x80;
	  }

	} break;

      }

      if( num_events >= 16 )
	break;
    }
  }

  return num_events;
}


/////////////////////////////////////////////////////////////////////////////
// Used for recording: insert an event into a selected step
// The return value matches with the layer where the new event has been inserted.
// if < 0, no event has been inserted.
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_RecEvent(u8 track, u16 step, seq_layer_evnt_t layer_event)
{
  seq_core_trk_t *t = &seq_core_trk[track];
  seq_cc_trk_t *tcc = &seq_cc_trk[track];

  if( tcc->event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(track); // we assume, that PAR layer has same number of instruments!

    // all events but Notes are ignored (CC/PitchBend/ProgramChange are working channel based, and not drum instr. based)
    if( layer_event.midi_package.event == NoteOn ) {
      u8 drum;
      for(drum=0; drum<num_instruments; ++drum) {
	u8 drum_note = tcc->lay_const[0*16 + drum];
	if( drum_note == layer_event.midi_package.note ) {

	  // BEGIN live pattern insertion
	  u8 gate = 1;
	  u8 accent = 0;
	  if( !seq_record_options.STEP_RECORD ) {
	    seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[1+drum];
	    if( slot->enabled ) {
	      seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
	      u16 mask = 1 << (step % 16);
	      gate = (pattern->gate & mask) ? 1 : 0;
	      accent = (pattern->accent & mask) ? 1 : 0;
	      layer_event.midi_package.velocity = slot->velocity;
	    }
	  }
	  // END live pattern insertion

	  SEQ_TRG_GateSet(track, step, drum, gate);
	  SEQ_TRG_AccentSet(track, step, drum, accent);
	  
	  int par_layer;
	  if( (par_layer=tcc->link_par_layer_velocity) >= 0 )
	    SEQ_PAR_Set(track, step, par_layer, drum, layer_event.midi_package.velocity);
	  
	  return drum; // drum note has been inserted - return instrument layer
	}
      }
    }
  } else {
    u8 instrument = 0;
    int par_layer;
    u8 num_p_layers = SEQ_PAR_NumLayersGet(track);

    // BEGIN live pattern insertion
    u8 gate = 1;
    u8 accent = 0;
    if( !seq_record_options.STEP_RECORD ) {
      seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[0];
      if( slot->enabled ) {
	seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
	u16 mask = 1 << (step % 16);
	gate = (pattern->gate & mask) ? 1 : 0;
	accent = (pattern->accent & mask) ? 1 : 0;
	layer_event.midi_package.velocity = slot->velocity;
	layer_event.len = slot->len;
      }
    }
    // END live pattern insertion

    // in poly mode: check if note already recorded
    if( seq_record_options.POLY_RECORD && layer_event.midi_package.event == NoteOn ) {
      for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
	seq_par_layer_type_t par_type = tcc->lay_const[0*16 + par_layer];
	if( (par_type == SEQ_PAR_Type_Note || par_type == SEQ_PAR_Type_Chord1 || par_type == SEQ_PAR_Type_Chord2 || par_type == SEQ_PAR_Type_Chord3) && 
	    SEQ_PAR_Get(track, step, par_layer, instrument) == layer_event.midi_package.note ) {

	  // set gate and take over new velocity/length (poly mode: last vel/length will be taken for all)
	  SEQ_TRG_GateSet(track, step, instrument, gate);
	  SEQ_TRG_AccentSet(track, step, instrument, accent);

	  if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	    // insert velocity into track 2/9
	    SEQ_PAR_Set(track+1, step, par_layer, instrument, layer_event.midi_package.velocity);
	    // insert length into track 3/10
	    SEQ_PAR_Set(track+2, step, par_layer, instrument, (layer_event.len >= 95) ? 95 : layer_event.len);
	  } else {
	    if( tcc->link_par_layer_velocity >= 0 )
	      SEQ_PAR_Set(track, step, tcc->link_par_layer_velocity, instrument, layer_event.midi_package.velocity);

	    if( tcc->link_par_layer_length >= 0 )
	      SEQ_PAR_Set(track, step, tcc->link_par_layer_length, instrument, (layer_event.len >= 95) ? 95 : layer_event.len);
	  }

	  // return the parameter layer
	  return par_layer;
	}
      }
    }

    // if gate isn't set or MONO mode: start poly counter at 0 (remaining notes will be cleared)
    if( !seq_record_options.POLY_RECORD || !SEQ_TRG_GateGet(track, step, instrument) )
      t->rec_poly_ctr = 0;

    // go through all layers to search for matching event
    u8 note_ctr = 0;
    for(par_layer=0; par_layer<num_p_layers; ++par_layer) {
      switch( tcc->lay_const[0*16 + par_layer] ) {

        case SEQ_PAR_Type_Note:
        case SEQ_PAR_Type_Chord1:
        case SEQ_PAR_Type_Chord2:
        case SEQ_PAR_Type_Chord3: {
	  if( layer_event.midi_package.event == NoteOn ) {

	    // in poly mode: skip if note number doesn't match with poly counter
	    if( seq_record_options.POLY_RECORD && note_ctr != t->rec_poly_ctr ) {
	      ++note_ctr;
	      break;
	    }

	    // if first note (reached again): clear remaining notes
	    // always active in mono mode (TODO: good? Or should we record on the selected par layer?)
	    if( !seq_record_options.POLY_RECORD || t->rec_poly_ctr == 0 ) {
	      u8 remaining_par_layer;
	      for(remaining_par_layer=par_layer+1; remaining_par_layer<num_p_layers; ++remaining_par_layer) {
		seq_par_layer_type_t par_type = tcc->lay_const[0*16 + remaining_par_layer];
		if( par_type == SEQ_PAR_Type_Note || par_type == SEQ_PAR_Type_Chord1 || par_type == SEQ_PAR_Type_Chord2 || par_type == SEQ_PAR_Type_Chord3 )
		  SEQ_PAR_Set(track, step, remaining_par_layer, instrument, 0x00);
	      }
	    }
	    SEQ_PAR_Set(track, step, par_layer, instrument, layer_event.midi_package.note);

	    // set gate and take over new velocity/length (poly mode: last vel/length will be taken for all)
	    SEQ_TRG_GateSet(track, step, instrument, gate);
	    SEQ_TRG_AccentSet(track, step, instrument, accent);

	    if( tcc->event_mode == SEQ_EVENT_MODE_Combined ) {
	      // insert velocity into track 2/9
	      SEQ_PAR_Set(track+1, step, par_layer, instrument, layer_event.midi_package.velocity);
	      // insert length into track 3/10
	      SEQ_PAR_Set(track+2, step, par_layer, instrument, (layer_event.len >= 95) ? 95 : layer_event.len);
	    } else {
	      if( tcc->link_par_layer_velocity >= 0 )
		SEQ_PAR_Set(track, step, tcc->link_par_layer_velocity, instrument, layer_event.midi_package.velocity);

	      if( tcc->link_par_layer_length >= 0 )
		SEQ_PAR_Set(track, step, tcc->link_par_layer_length, instrument, (layer_event.len >= 95) ? 95 : layer_event.len);
	    }

	    // in poly mode: continue search for next free note, wrap if end of layer is reached
	    if( seq_record_options.POLY_RECORD ) {
	      u8 remaining_par_layer;
	      for(remaining_par_layer=par_layer+1; remaining_par_layer<num_p_layers; ++remaining_par_layer) {
		seq_par_layer_type_t par_type = tcc->lay_const[0*16 + remaining_par_layer];
		if( par_type == SEQ_PAR_Type_Note || par_type == SEQ_PAR_Type_Chord1 || par_type == SEQ_PAR_Type_Chord2 || par_type == SEQ_PAR_Type_Chord3 ) {
		  ++t->rec_poly_ctr; // switch to next note
		  break;
		}
	      }

	      // out of notes - restart at 0
	      if( remaining_par_layer >= num_p_layers )
		t->rec_poly_ctr = 0;
	    }

	    return par_layer;
	  }
	} break;

        case SEQ_PAR_Type_CC: {
	  if( layer_event.midi_package.event == CC && layer_event.midi_package.cc_number == tcc->lay_const[1*16 + par_layer] ) {
	    // extra MBSEQ V4L: write into whole 16th step in step record mode
	    if(
#ifndef MBSEQV4L
	       0
#else
	       (seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning()) && tcc->clkdiv.value == 0x03
#endif
	       ) {
	      int i;
	      for(i=0; i<4; ++i)
		SEQ_PAR_Set(track, step*4+i, par_layer, instrument, layer_event.midi_package.value);
	    } else {
	      // Live Record mode: write into the next 4*resolution steps (till end of track)
	      int i;
	      u16 num_p_steps = SEQ_PAR_NumStepsGet(track);
#ifndef MBSEQV4L
	      int num_steps = 1;
#else
	      int num_steps = 4;
#endif

	      if( !seq_record_options.STEP_RECORD ) {
		num_steps = 4;
		if( tcc->clkdiv.value <= 0x03 )
		  num_steps = 16;
		else if( tcc->clkdiv.value <= 0x07 )
		  num_steps = 8;
	      }

	      for(i=0; i<num_steps && (step+i) < num_p_steps; ++i) {
		SEQ_PAR_Set(track, step+i, par_layer, instrument, layer_event.midi_package.value);
	      }
	    }
	    return par_layer;
	  }
	} break;

        case SEQ_PAR_Type_PitchBend: {
	  if( layer_event.midi_package.event == PitchBend ) {
	    // extra MBSEQ V4L: write into whole 16th step in step record mode
	    if(
#ifndef MBSEQV4L
	       0
#else
	       (seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning()) && tcc->clkdiv.value == 0x03
#endif
	       ) {
	      int i;
	      for(i=0; i<4; ++i)
		SEQ_PAR_Set(track, step*4+i, par_layer, instrument, layer_event.midi_package.evnt2); // MSB
	    } else {
	      // Live Record mode: write into the next 4*resolution steps (till end of track)
	      int i;
	      u16 num_p_steps = SEQ_PAR_NumStepsGet(track);
	      int num_steps = 4;
	      if( tcc->clkdiv.value <= 0x03 )
		num_steps = 16;
	      else if( tcc->clkdiv.value <= 0x07 )
		num_steps = 8;
	      for(i=0; i<num_steps && (step+i) < num_p_steps; ++i) {
		SEQ_PAR_Set(track, step+i, par_layer, instrument, layer_event.midi_package.evnt2); // MSB
	      }
	      return par_layer;
	    }
	  }
	} break;

        case SEQ_PAR_Type_ProgramChange: {
	  if( layer_event.midi_package.event == ProgramChange ) {
	    // extra MBSEQ V4L: write into whole 16th step in step record mode
	    if(
#ifndef MBSEQV4L
	       0
#else
	       (seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning()) && tcc->clkdiv.value == 0x03
#endif
	       ) {
	      int i;
	      for(i=0; i<4; ++i)
		SEQ_PAR_Set(track, step*4+i, par_layer, instrument, layer_event.midi_package.evnt1);
	    } else {
	      SEQ_PAR_Set(track, step, par_layer, instrument, layer_event.midi_package.evnt1);
	      return par_layer;
	    }
	  }
	} break;

        case SEQ_PAR_Type_Aftertouch: {
	  if( layer_event.midi_package.event == Aftertouch ) {
	    // extra MBSEQ V4L: write into whole 16th step in step record mode
	    if(
#ifndef MBSEQV4L
	       0
#else
	       (seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning()) && tcc->clkdiv.value == 0x03
#endif
	       ) {
	      int i;
	      for(i=0; i<4; ++i)
		SEQ_PAR_Set(track, step*4+i, par_layer, instrument, layer_event.midi_package.evnt1);
	    } else {
	      SEQ_PAR_Set(track, step, par_layer, instrument, layer_event.midi_package.evnt1);
	      return par_layer;
	    }
	  } else if( layer_event.midi_package.event == PolyPressure ) {
	    // extra MBSEQ V4L: write into whole 16th step in step record mode
	    if(
#ifndef MBSEQV4L
	       0
#else
	       (seq_record_options.STEP_RECORD || !SEQ_BPM_IsRunning()) && tcc->clkdiv.value == 0x03
#endif
	       ) {
	      int i;
	      for(i=0; i<4; ++i)
		SEQ_PAR_Set(track, step*4+i, par_layer, instrument, layer_event.midi_package.evnt2);
	    } else {
	      SEQ_PAR_Set(track, step, par_layer, instrument, layer_event.midi_package.evnt2);
	      return par_layer;
	    }
	  }
	} break;
      }
    }
  }

  return -1; // no matching event found!
}


/////////////////////////////////////////////////////////////////////////////
// Sends the last CC/PB/PC value of the given track and layer
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_DirectSendEvent(u8 track, u8 par_layer)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];
  seq_par_layer_type_t layer_type = tcc->lay_const[par_layer];
  mios32_midi_package_t p;

  p.cable    = track;
  p.chn      = tcc->midi_chn;

  switch( layer_type ) {
  case SEQ_PAR_Type_CC: {
    u8 cc_number = tcc->lay_const[1*16 + par_layer];
    u8 value = (cc_last_value[track][0][par_layer] < 0x80) ? cc_last_value[track][0][par_layer] : 0x40;

    if( cc_number >= 0x80 )
      return -1; // CC disabled

    p.type      = CC;
    p.event     = CC;
    p.cc_number = tcc->lay_const[1*16 + par_layer];
    p.value     = value;
  } break;

  case SEQ_PAR_Type_PitchBend: {
    u8 value = (pb_last_value[track] < 0x80) ? pb_last_value[track] : 0x40;

    p.type      = PitchBend;
    p.event     = PitchBend;
    p.evnt1     = (value == 0x40) ? 0x00 : value; // LSB
    p.evnt2     = value;
  } break;

  case SEQ_PAR_Type_ProgramChange: {
    u8 value = (pc_last_value[track] < 0x80) ? pc_last_value[track] : 0;

    p.type      = ProgramChange;
    p.event     = ProgramChange;
    p.evnt1     = value;
    p.evnt2     = 0x00; // don't care
  } break;

  case SEQ_PAR_Type_Aftertouch: {
    u8 value = (at_last_value[track] < 0x80) ? at_last_value[track] : 0;

    p.type      = Aftertouch;
    p.event     = Aftertouch;
    p.evnt1     = value;
    p.evnt2     = 0x00; // don't care
  } break;

  default:
    return -1; // other parameter types are not supported
  }

  // send MIDI event
  MUTEX_MIDIOUT_TAKE;
  MIOS32_MIDI_SendPackage(tcc->midi_port, p);
  MUTEX_MIDIOUT_GIVE;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initializes a track depending on selected event mode
// "only_layers" flag is used by CLR function
// "all_triggers_cleared": if 0, triggers will be initialized according to preset
//                         if 1: triggers will be cleared
// "init_assignments": if 1, parameter/trigger assignments will be changed
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_CopyPreset(u8 track, u8 only_layers, u8 all_triggers_cleared, u8 init_assignments)
{
  int i;
  int cc;

  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);

  // layer specific mutes always cleared to avoid confusion!
  seq_core_trk[track].layer_muted = 0;

  if( !only_layers ) {
    // copy static presets
    i = 0;
    while( (cc=seq_layer_preset_table_static[i][0]) != 0xff ) {
      SEQ_CC_Set(track, cc, seq_layer_preset_table_static[i][1]);
      i++;
    }

    // initialize assignments
    if( init_assignments ) {
      switch( event_mode ) {
        case SEQ_EVENT_MODE_Note:
        case SEQ_EVENT_MODE_Chord:
        case SEQ_EVENT_MODE_Combined: {
	  // Trigger Layer Assignments
	  for(i=0; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, i+1);

	  // Parameter Layer Assignments
	  if( event_mode == SEQ_EVENT_MODE_Combined ) {
	    if( (track&7) == 0 ) {
	      for(i=0; i<16; ++i)
		SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_Note);
	    } else if( (track&7) == 1 ) {
	      for(i=0; i<16; ++i)
		SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_Velocity);
	    } else if( (track&7) == 2 ) {
	      for(i=0; i<16; ++i)
		SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_Length);
	    }
	  } else {
	    // Parameter Layer Assignments
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1, (event_mode == SEQ_EVENT_MODE_Chord) ? SEQ_PAR_Type_Chord1 : SEQ_PAR_Type_Note);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A2, SEQ_PAR_Type_Velocity);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A3, SEQ_PAR_Type_Length);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A4, SEQ_PAR_Type_Roll);
	    for(i=4; i<16; ++i)
	      SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_Note);
	  }

	  // new: assign disabled CCs by default
	  for(i=0; i<16; ++i)
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 0x80);

        } break;

        case SEQ_EVENT_MODE_CC: {
	  // Trigger Layer Assignments
	  for(i=0; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, i+1);

	  // Parameter Layer Assignments
	  for(i=0; i<16; ++i)
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+i, SEQ_PAR_Type_CC);

#ifndef MBSEQV4L
	  for(i=0; i<16; ++i) // disable all CCs by default
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 0x80);
#else
	  // extra for MBSEQ V4L:
	  // CCs disabled and will be assigned during recording
	  for(i=0; i<16; ++i)
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 0x80);

	  // G1T4 and G3T4: first layer assigned to pitchbender
	  if( track == 3 || track == 11 )
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+0, SEQ_PAR_Type_PitchBend);

	  // running with 4x resolution
	  SEQ_CC_Set(track, SEQ_CC_CLK_DIVIDER, 0x03);
	  SEQ_CC_Set(track, SEQ_CC_LENGTH, 0x3f);
#endif
        } break;

        case SEQ_EVENT_MODE_Drum: {
	  // Trigger Layer Assignments
	  SEQ_CC_Set(track, SEQ_CC_ASG_GATE, 1);
	  SEQ_CC_Set(track, SEQ_CC_ASG_ACCENT, (SEQ_TRG_NumLayersGet(track) > 1) ? 2 : 0);
	  for(i=2; i<8; ++i)
	    SEQ_CC_Set(track, SEQ_CC_ASG_GATE+i, 0); // not relevant in drum mode

	  // parameter layer assignments
	  if( SEQ_PAR_NumLayersGet(track) > 1 ) {
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Velocity);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_B, SEQ_PAR_Type_Roll);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_C, SEQ_PAR_Type_Probability);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_D, SEQ_PAR_Type_Nth1);
	  } else if( SEQ_TRG_NumLayersGet(track) > 1 ) {
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Roll);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_B, SEQ_PAR_Type_None);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_C, SEQ_PAR_Type_None);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_D, SEQ_PAR_Type_None);
	  } else {
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_A, SEQ_PAR_Type_Velocity);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_B, SEQ_PAR_Type_None);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_C, SEQ_PAR_Type_None);
	    SEQ_CC_Set(track, SEQ_CC_PAR_ASG_DRUM_LAYER_D, SEQ_PAR_Type_None);
	  }

	  // Constant Layer Values
	  int drum;
	  for(drum=0; drum<16; ++drum) {
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+drum, seq_layer_preset_table_drum_notes[drum]);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+drum, 100);
	    SEQ_CC_Set(track, SEQ_CC_LAY_CONST_C1+drum, 127);
	  }
	} break;
      }
    }

    // copy event mode depending settings
    switch( event_mode ) {
      case SEQ_EVENT_MODE_Note:
      case SEQ_EVENT_MODE_Chord:
      case SEQ_EVENT_MODE_CC:
      case SEQ_EVENT_MODE_Combined: {
	// CCs disabled and will be assigned in INIT page or during recording
	for(i=0; i<16; ++i)
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+i, 0x80);
        } break;

      case SEQ_EVENT_MODE_Drum: {
	int drum;
	for(drum=0; drum<16; ++drum) {
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_A1+drum, seq_layer_preset_table_drum_notes[drum]);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_B1+drum, 100);
	  SEQ_CC_Set(track, SEQ_CC_LAY_CONST_C1+drum, 127);
	}
      } break;
    }
  }


  // get constraints of trigger layers
  int num_t_steps  = SEQ_TRG_NumStepsGet(track);;

  // copy trigger layer values
  if( !all_triggers_cleared ) {
    int step8;
    u8 instrument = 0;
    u8 layer = 0;

    if( event_mode == SEQ_EVENT_MODE_CC ) {
      // CC: enable gate for all steps
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, layer, instrument, 0xff);
    } else if( event_mode == SEQ_EVENT_MODE_Combined ) {
      // Combined: clear all gates
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, layer, instrument, 0x00);
    } else {
      // Note/Chord/Drum: enable trigger for each beat of (first) gate layer
      // New: can be disabled with INIT_WITH_TRIGGERS = 0 (configureable in OPTIONS menu)
      for(step8=0; step8<(num_t_steps/8); ++step8)
	SEQ_TRG_Set8(track, step8, layer, instrument, seq_core_options.INIT_WITH_TRIGGERS ? 0x11 : 0x00);
    }
  }


  // copy parameter layer values
  int par_layer;
  int num_p_layers = SEQ_PAR_NumLayersGet(track);
  for(par_layer=0; par_layer<num_p_layers; ++par_layer)
    SEQ_LAYER_CopyParLayerPreset(track, par_layer);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copies parameter layer preset depending on selected parameter type
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_LAYER_CopyParLayerPreset(u8 track, u8 par_layer)
{
  int num_p_instruments = SEQ_PAR_NumInstrumentsGet(track);
  int num_p_steps  = SEQ_PAR_NumStepsGet(track);;
  u8 init_value = SEQ_PAR_InitValueGet(SEQ_PAR_AssignmentGet(track, par_layer), par_layer);

  int step;
  int instrument;
  for(instrument=0; instrument<num_p_instruments; ++instrument)
    for(step=0; step<num_p_steps; ++step)
      SEQ_PAR_Set(track, step, par_layer, instrument, init_value);

  return 0; // no error
}
