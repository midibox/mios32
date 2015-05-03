// $Id$
/*
 * Track record page
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

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_trg.h"
#include "seq_par.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_record.h"
#include "seq_live.h"
#include "seq_file.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define SUBPAGE_REC_STEP  0
#define SUBPAGE_REC_LIVE  1
#define SUBPAGE_REC_PTN   2
#define SUBPAGE_MIDI      3
#define SUBPAGE_MISC      4
#define SUBPAGE_PATSEL    5


#define ITEM_GXTY               0
#define ITEM_REC_ENABLE         1
#define ITEM_FWD_ENABLE         2
#define ITEM_STEP_POLY_MODE     3
#define ITEM_STEP_AUTO_START    4
#define ITEM_STEP_RECORD_STEP   5
#define ITEM_STEP_RECORD_INC    6
#define ITEM_STEP_TOGGLE_GATE   7

#define FIRST_ITEM_STEP ITEM_STEP_POLY_MODE
#define LAST_ITEM_STEP ITEM_STEP_TOGGLE_GATE

#define ITEM_LIVE_POLY_MODE     8
#define ITEM_LIVE_AUTO_START    9
#define ITEM_LIVE_QUANTIZE     10

#define FIRST_ITEM_LIVE ITEM_LIVE_POLY_MODE
#define LAST_ITEM_LIVE ITEM_LIVE_QUANTIZE

#define ITEM_PTN_POLY_OR_DRUM  11
#define ITEM_PTN_ENABLE        12
#define ITEM_PTN_PATTERN       13
#define ITEM_PTN_LENGTH        14

#define FIRST_ITEM_PTN ITEM_PTN_POLY_OR_DRUM
#define LAST_ITEM_PTN ITEM_PTN_LENGTH

#define ITEM_MIDI_IN_BUS       15
#define ITEM_MIDI_IN_PORT      16
#define ITEM_MIDI_IN_CHN       17
#define ITEM_MIDI_IN_LOWER     18
#define ITEM_MIDI_IN_UPPER     19
#define ITEM_MIDI_IN_MODE      20
#define ITEM_MIDI_RESET_STACKS 21

#define FIRST_ITEM_MIDI ITEM_MIDI_IN_BUS
#define LAST_ITEM_MIDI ITEM_MIDI_RESET_STACKS

#define ITEM_MISC_OCT_TRANSPOSE 22
#define ITEM_MISC_FX_ENABLE     23
#define ITEM_MISC_FTS           24

#define FIRST_ITEM_MISC ITEM_MISC_OCT_TRANSPOSE
#define LAST_ITEM_MISC ITEM_MISC_FTS

#define NUM_OF_ITEMS           25


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 selected_subpage = SUBPAGE_REC_STEP;

static u8 selected_bus = 0;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 GetNumNoteLayers(void);
static s32 SEQ_UI_TRKJAM_Hlp_PrintPattern(u8 pattern);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  // branch to edit page if new event has been recorded (ui_hold_msg_ctr controlled from SEQ_RECORD_*)
  if( ui_hold_msg_ctr )
    return SEQ_UI_EDIT_LED_Handler(gp_leds);

  // print edit pattern
  if( selected_subpage == SUBPAGE_PATSEL || (selected_subpage == SUBPAGE_REC_PTN && seq_ui_button_state.SELECT_PRESSED) ) {
    seq_live_pattern_slot_t *slot = SEQ_LIVE_CurrentSlotGet();
    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
    *gp_leds = pattern->gate;
    return 0; // no error
  }

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  if( selected_subpage < 5 ) {
    *gp_leds = 1 << (selected_subpage + 3);
  }

  switch( selected_subpage ) {
  case SUBPAGE_REC_STEP:
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_REC_ENABLE: *gp_leds |= 0x0002; break;
    case ITEM_FWD_ENABLE: *gp_leds |= 0x0004; break;
    case ITEM_STEP_POLY_MODE: *gp_leds |= 0x0100; break;
    case ITEM_STEP_AUTO_START: *gp_leds |= 0x0200; break;
    case ITEM_STEP_RECORD_STEP: *gp_leds |= 0x0400; break;
    case ITEM_STEP_RECORD_INC: *gp_leds |= 0x0800; break;
    case ITEM_STEP_TOGGLE_GATE: *gp_leds |= 0x8000; break;
    }
    break;

  case SUBPAGE_REC_LIVE:
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_REC_ENABLE: *gp_leds |= 0x0002; break;
    case ITEM_FWD_ENABLE: *gp_leds |= 0x0004; break;
    case ITEM_LIVE_POLY_MODE: *gp_leds |= 0x0100; break;
    case ITEM_LIVE_AUTO_START: *gp_leds |= 0x0200; break;
    case ITEM_LIVE_QUANTIZE: *gp_leds |= 0x0c00; break;
    }
    break;

  case SUBPAGE_REC_PTN:
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_REC_ENABLE: *gp_leds |= 0x0002; break;
    case ITEM_FWD_ENABLE: *gp_leds |= 0x0004; break;
    case ITEM_PTN_POLY_OR_DRUM: *gp_leds |= 0x0100; break;
    case ITEM_PTN_ENABLE: *gp_leds |= 0x0200; break;
    case ITEM_PTN_PATTERN: *gp_leds |= 0x1c00; break;
    case ITEM_PTN_LENGTH: *gp_leds |= 0x2000; break;
    }
    break;

  case SUBPAGE_MIDI:
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_REC_ENABLE: *gp_leds |= 0x0002; break;
    case ITEM_FWD_ENABLE: *gp_leds |= 0x0004; break;
    case ITEM_MIDI_IN_BUS: *gp_leds |= 0x0100; break;
    case ITEM_MIDI_IN_PORT: *gp_leds |= 0x0200; break;
    case ITEM_MIDI_IN_CHN: *gp_leds |= 0x0400; break;
    case ITEM_MIDI_IN_LOWER: *gp_leds |= 0x0800; break;
    case ITEM_MIDI_IN_UPPER: *gp_leds |= 0x1000; break;
    case ITEM_MIDI_IN_MODE: *gp_leds |= 0x2000; break;
    case ITEM_MIDI_RESET_STACKS: *gp_leds |= 0x8000; break;
    }
    break;

  case SUBPAGE_MISC:
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_REC_ENABLE: *gp_leds |= 0x0002; break;
    case ITEM_FWD_ENABLE: *gp_leds |= 0x0004; break;
    case ITEM_MISC_OCT_TRANSPOSE: *gp_leds |= 0x0100; break;
    case ITEM_MISC_FX_ENABLE: *gp_leds |= 0x0200; break;
    case ITEM_MISC_FTS: *gp_leds |= 0x0400; break;
    }
    break;
  }

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Help function
/////////////////////////////////////////////////////////////////////////////
static s32 SetSubpageBasedOnItem(u8 item)
{
  if( item <= LAST_ITEM_STEP )
    selected_subpage = SUBPAGE_REC_STEP;
  else if( item <= LAST_ITEM_LIVE )
    selected_subpage = SUBPAGE_REC_LIVE;
  else if( item <= LAST_ITEM_PTN )
    selected_subpage = SUBPAGE_REC_PTN;
  else if( item <= LAST_ITEM_MIDI )
    selected_subpage = SUBPAGE_MIDI;
  else
    selected_subpage = SUBPAGE_MISC;

  seq_record_options.STEP_RECORD = (selected_subpage == SUBPAGE_REC_STEP);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local encoder callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported encoder
/////////////////////////////////////////////////////////////////////////////
static s32 Encoder_Handler(seq_ui_encoder_t encoder, s32 incrementer)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 ix = (event_mode == SEQ_EVENT_MODE_Drum) ? (1+ui_selected_instrument) : 0;
  seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[ix];    

  // ensure that original record screen will be print immediately
  ui_hold_msg_ctr = 0;

  if( (selected_subpage == SUBPAGE_PATSEL || (selected_subpage == SUBPAGE_REC_PTN && seq_ui_button_state.SELECT_PRESSED)) && encoder < 16 ) {
    if( slot->pattern >= SEQ_LIVE_NUM_ARP_PATTERNS )
      return -1; // invalid pattern

    // in PATSEL page: only encoders change the gate/accent
    // buttons select on/off, pattern and exit
    if( selected_subpage == SUBPAGE_PATSEL && incrementer == 0 ) {
      if( encoder == SEQ_UI_ENCODER_GP15 ) { // actually SEQ_UI_BUTTON_GP15
	selected_subpage = SUBPAGE_REC_PTN;
      } else if( encoder == SEQ_UI_ENCODER_GP16 ) { // actually SEQ_UI_BUTTON_GP16
	slot->enabled = slot->enabled ? 0 : 1;
      } else {
	slot->pattern = encoder;
      }
      return 1; // key taken
    }

    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
    u16 mask = (1 << encoder);
    u8 gate = (pattern->gate & mask) ? 1 : 0;
    u8 accent = (pattern->accent & mask) ? 3 : 0; // and not 3: we want to toggle between 0, 1 and 2; accent set means that gate is set as well
    u8 value = gate | accent;
    if( value >= 2 ) value = 2;

    if( incrementer == 0 ) {
      // button toggles between values
      if( ++value >= 3 )
	value = 0;
    } else {
      // encoder moves between 3 states
      if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) <= 0 ) {
	return -1;
      }
    }

    if( value == 0 ) {
      pattern->gate &= ~mask;
      pattern->accent &= ~mask;
    } else if( value == 1 ) {
      pattern->gate |= mask;
      pattern->accent &= ~mask;
    } else if( value == 2 ) {
      pattern->gate |= mask;
      pattern->accent |= mask;
    }
    ui_store_file_required = 1;

    return 1;
  }

  if( encoder <= SEQ_UI_ENCODER_GP8 ) {
    switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_REC_ENABLE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_FWD_ENABLE;
      break;

    case SEQ_UI_ENCODER_GP4:
      selected_subpage = SUBPAGE_REC_STEP;
      ui_selected_item = FIRST_ITEM_STEP;
      seq_record_options.STEP_RECORD = 1;
      return 1;

    case SEQ_UI_ENCODER_GP5:
      selected_subpage = SUBPAGE_REC_LIVE;
      ui_selected_item = FIRST_ITEM_LIVE;
      seq_record_options.STEP_RECORD = 0;
      return 1;

    case SEQ_UI_ENCODER_GP6:
      selected_subpage = SUBPAGE_REC_PTN;
      ui_selected_item = FIRST_ITEM_PTN;
      seq_record_options.STEP_RECORD = 0;
      return 1;

    case SEQ_UI_ENCODER_GP7:
      selected_subpage = SUBPAGE_MIDI;
      ui_selected_item = FIRST_ITEM_MIDI;
      seq_record_options.STEP_RECORD = 0;
      return 1;

    case SEQ_UI_ENCODER_GP8:
      selected_subpage = SUBPAGE_MISC;
      ui_selected_item = FIRST_ITEM_MISC;
      seq_record_options.STEP_RECORD = 0;
      return 1;
    }
  } else if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    switch( selected_subpage ) {
    ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_REC_STEP: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_STEP_POLY_MODE;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = ITEM_STEP_AUTO_START;
	break;

      case SEQ_UI_ENCODER_GP11:
	ui_selected_item = ITEM_STEP_RECORD_STEP;
	break;

      case SEQ_UI_ENCODER_GP12:
	ui_selected_item = ITEM_STEP_RECORD_INC;
	break;

      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
	return -1; // not used (yet)

      case SEQ_UI_ENCODER_GP16:
	ui_selected_item = ITEM_STEP_TOGGLE_GATE;
	break;
      }
    } break;

    ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_REC_LIVE: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_LIVE_POLY_MODE;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = ITEM_LIVE_AUTO_START;
	break;

      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12:
	ui_selected_item = ITEM_LIVE_QUANTIZE;
	break;

      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16:
	return -1; // not used (yet)
      }
    } break;

    ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_REC_PTN: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_PTN_POLY_OR_DRUM;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = ITEM_PTN_ENABLE;
	break;

      case SEQ_UI_ENCODER_GP11:
      case SEQ_UI_ENCODER_GP12:
      case SEQ_UI_ENCODER_GP13:
	ui_selected_item = ITEM_PTN_PATTERN;

	// button pressed -> change to PATSEL page
	if( incrementer == 0 ) {
	  selected_subpage = SUBPAGE_PATSEL;
	  return 1;
	}
	break;

      case SEQ_UI_ENCODER_GP14:
	ui_selected_item = ITEM_PTN_LENGTH;
	break;

      case SEQ_UI_ENCODER_GP15:
	SEQ_UI_UTIL_CopyLivePattern();
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Pattern copied", "into Edit Buffer");
	return 1;

      case SEQ_UI_ENCODER_GP16:
	if( SEQ_UI_UTIL_PasteLivePattern() < 0 ) {
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Edit Buffer", "is empty!");
	} else {
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Pattern copied", "from Edit Buffer");
	}
	return 1;
      }
    } break;

    ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_MIDI: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_MIDI_IN_BUS;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = ITEM_MIDI_IN_PORT;
	break;

      case SEQ_UI_ENCODER_GP11:
	ui_selected_item = ITEM_MIDI_IN_CHN;
	break;

      case SEQ_UI_ENCODER_GP12:
	ui_selected_item = ITEM_MIDI_IN_LOWER;
	break;

      case SEQ_UI_ENCODER_GP13:
	ui_selected_item = ITEM_MIDI_IN_UPPER;
	break;

      case SEQ_UI_ENCODER_GP14:
	ui_selected_item = ITEM_MIDI_IN_MODE;
	break;

      case SEQ_UI_ENCODER_GP15:
	return -1; // not mapped

      case SEQ_UI_ENCODER_GP16:
	ui_selected_item = ITEM_MIDI_RESET_STACKS;
	break;
      }
    } break;

    ///////////////////////////////////////////////////////////////////////////
    case SUBPAGE_MISC: {
      switch( encoder ) {
      case SEQ_UI_ENCODER_GP9:
	ui_selected_item = ITEM_MISC_OCT_TRANSPOSE;
	break;

      case SEQ_UI_ENCODER_GP10:
	ui_selected_item = ITEM_MISC_FX_ENABLE;
	break;

      case SEQ_UI_ENCODER_GP11:
	ui_selected_item = ITEM_MISC_FTS;
	break;

      case SEQ_UI_ENCODER_GP12:
      case SEQ_UI_ENCODER_GP13:
      case SEQ_UI_ENCODER_GP14:
      case SEQ_UI_ENCODER_GP15:
      case SEQ_UI_ENCODER_GP16:
	return -1; // not used (yet)
      }
    } break;
    }
  }


  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
  case ITEM_GXTY:
    return SEQ_UI_GxTyInc(incrementer);

  case ITEM_REC_ENABLE: {
    u8 value = seq_record_state.ENABLED;
    if( incrementer == 0 )
      value = value ? 0 : 1;
    else
      value = (incrementer > 0);

    if( value != seq_record_state.ENABLED ) {
      SEQ_RECORD_Enable(value, 1);
      return 1;
    }
    return 0;
  }

  case ITEM_FWD_ENABLE: {
    if( incrementer == 0 )
      seq_record_options.FWD_MIDI = seq_record_options.FWD_MIDI ? 0 : 1;
    else
      seq_record_options.FWD_MIDI = (incrementer > 0);
    return 1;
  }

  case ITEM_PTN_POLY_OR_DRUM: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      u8 num_drums = SEQ_TRG_NumInstrumentsGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_drums-1, incrementer);
    }
  }
    // no break!
    // fall through POLY mode selection

  case ITEM_STEP_POLY_MODE:
  case ITEM_LIVE_POLY_MODE: {
    if( incrementer )
      seq_record_options.POLY_RECORD = incrementer > 0 ? 1 : 0;
    else
      seq_record_options.POLY_RECORD ^= 1;

    ui_store_file_required = 1;

    // print warning if poly mode not possible
    if( seq_record_options.POLY_RECORD ) {
      int num_note_layers = GetNumNoteLayers();
      if( num_note_layers < 2 ) {
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "No Poly Recording:",
		   num_note_layers ? "Track has only one Note Layer!" : "Track has no Note Layers!");
      }
    }
    return 1;
  }

  case ITEM_STEP_AUTO_START:
  case ITEM_LIVE_AUTO_START: {
    if( incrementer )
      seq_record_options.AUTO_START = incrementer > 0 ? 1 : 0;
    else
      seq_record_options.AUTO_START ^= 1;
    ui_store_file_required = 1;
    return 1;
  }

  case ITEM_STEP_RECORD_STEP: {
    u8 step = ui_selected_step;
    if( SEQ_UI_Var8_Inc(&step, 0, SEQ_CC_Get(visible_track, SEQ_CC_LENGTH), incrementer) > 0 ) {
      u8 track;
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
	SEQ_RECORD_Reset(track);

      ui_selected_step = step;

      // print edit screen
      SEQ_RECORD_PrintEditScreen();

      return 1;
    }
    return 0;
  }

  case ITEM_STEP_RECORD_INC: {
    if( !seq_record_options.POLY_RECORD ) {
      u8 value = seq_record_options.STEPS_PER_KEY;
      if( SEQ_UI_Var8_Inc(&value, 0, 16, incrementer) > 0 ) {
	seq_record_options.STEPS_PER_KEY = value;
	ui_store_file_required = 1;
	return 1;
      }
    }
    return 0;
  }

  case ITEM_STEP_TOGGLE_GATE: {
    // toggle gate of current step
    u8 gate;
#if 0
    // not logical in combination with increment step
    if( incrementer > 0 )
      gate = 1;
    else if( incrementer < 0 )
      gate = 0;
    else
#endif
      gate = SEQ_TRG_GateGet(visible_track, ui_selected_step, ui_selected_instrument) ? 0 : 1;

    int i;
    for(i=0; i<SEQ_TRG_NumInstrumentsGet(visible_track); ++i)
      SEQ_TRG_GateSet(visible_track, ui_selected_step, i, gate);

    // increment step
    int next_step = (ui_selected_step + seq_record_options.STEPS_PER_KEY) % ((int)SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1);

    for(i=0; i<SEQ_CORE_NUM_TRACKS; ++i)
      SEQ_RECORD_Reset(i);

    ui_selected_step = next_step;

    // print edit screen
    SEQ_RECORD_PrintEditScreen();

    return 1;
  }

  case ITEM_LIVE_QUANTIZE: {
    if( SEQ_UI_Var8_Inc(&seq_record_quantize, 0, 99, incrementer) >= 0 ) {
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  }

  case ITEM_PTN_ENABLE: {
    if( incrementer == 0 )
      slot->enabled = slot->enabled ? 0 : 1;
    else
      slot->enabled = (incrementer > 0);
    return 1;
  }

  case ITEM_PTN_PATTERN: {
    u8 value = slot->pattern;
    if( SEQ_UI_Var8_Inc(&value, 0, SEQ_LIVE_NUM_ARP_PATTERNS-1, incrementer) > 0 ) {
      slot->pattern = value;
      return 1;
    }
    return 0;
  }

  case ITEM_PTN_LENGTH: {
    u8 value = slot->len;
    if( SEQ_UI_Var8_Inc(&value, 0, 94, incrementer) > 0 ) {
      slot->len = value;
      return 1;
    }
    return 0;
  }

  case ITEM_MIDI_IN_BUS: {
    if( SEQ_UI_Var8_Inc(&selected_bus, 0, SEQ_MIDI_IN_NUM_BUSSES-1, incrementer) >= 0 ) {
      return 1; // value changed
    }
    return 0; // no change
  }

  case ITEM_MIDI_IN_PORT: {
    u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus]);
    if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1-4, incrementer) >= 0 ) { // don't allow selection of Bus1..Bus4
      seq_midi_in_port[selected_bus] = SEQ_MIDI_PORT_InPortGet(port_ix);
      SEQ_RECORD_AllNotesOff(); // reset note markers
      SEQ_LIVE_AllNotesOff();
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  } break;

  case ITEM_MIDI_IN_CHN: {
    if( SEQ_UI_Var8_Inc(&seq_midi_in_channel[selected_bus], 0, 17, incrementer) >= 0 ) {
      ui_store_file_required = 1;
      SEQ_RECORD_AllNotesOff(); // reset note markers
      SEQ_LIVE_AllNotesOff();
      return 1; // value changed
    }
    return 0; // no change
  }

  case ITEM_MIDI_IN_LOWER: {
    if( SEQ_UI_Var8_Inc(&seq_midi_in_lower[selected_bus], 0, 127, incrementer) >= 0 ) {
      SEQ_RECORD_AllNotesOff(); // reset note markers
      SEQ_LIVE_AllNotesOff();
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  }

  case ITEM_MIDI_IN_UPPER: {
    if( SEQ_UI_Var8_Inc(&seq_midi_in_upper[selected_bus], 0, 127, incrementer) >= 0 ) {
      SEQ_RECORD_AllNotesOff(); // reset note markers
      SEQ_LIVE_AllNotesOff();
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  }

  case ITEM_MIDI_IN_MODE: {
    u8 fwd = seq_midi_in_options[selected_bus].MODE_PLAY;
    if( incrementer == 0 )
      incrementer = fwd ? -1 : 1;
    if( SEQ_UI_Var8_Inc(&fwd, 0, 1, incrementer) >= 0 ) {
      seq_midi_in_options[selected_bus].MODE_PLAY = fwd;
      SEQ_RECORD_AllNotesOff(); // reset note markers
      SEQ_LIVE_AllNotesOff();
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  } break;

  case ITEM_MIDI_RESET_STACKS: {
    SEQ_MIDI_IN_ResetTransArpStacks();
    SEQ_RECORD_AllNotesOff(); // reset note markers
    SEQ_LIVE_AllNotesOff();
    SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Transposer/Arp.", "Stacks cleared!");
    return 1;
  } break;

  case ITEM_MISC_OCT_TRANSPOSE: {
    if( event_mode == SEQ_EVENT_MODE_Drum )
      return -1; // disabled

    u8 tmp = seq_live_options.OCT_TRANSPOSE + 5;
    if( SEQ_UI_Var8_Inc(&tmp, 0, 10, incrementer) >= 0 ) {
      seq_live_options.OCT_TRANSPOSE = (s8)tmp - 5;
      ui_store_file_required = 1;
      return 1; // value changed
    }
    return 0; // no change
  } break;

  case ITEM_MISC_FX_ENABLE: {
    if( incrementer )
      seq_live_options.FX = incrementer > 0 ? 1 : 0;
    else
      seq_live_options.FX ^= 1;
    ui_store_file_required = 1;
    return 1;
  }

  case ITEM_MISC_FTS: {
    if( incrementer )
      seq_live_options.FORCE_SCALE = incrementer > 0 ? 1 : 0;
    else
      seq_live_options.FORCE_SCALE ^= 1;
    ui_store_file_required = 1;
    return 1;
  }
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
  if( depressed ) return 0; // ignore when button depressed

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // re-use encoder handler - only select UI item, don't increment, flags will be toggled
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      SetSubpageBasedOnItem(ui_selected_item);
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      SetSubpageBasedOnItem(ui_selected_item);
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, -1);
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // branch to edit page if new event has been recorded (ui_hold_msg_ctr controlled from SEQ_RECORD_*)
  if( ui_hold_msg_ctr )
    return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_RECORD);

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Rec. Fwd.    Configuration Pages   Mode AStart Step Inc.             Toggle
  // G1T1 off   on >Step<Live Ptn. MIDI Misc Poly  on      1  + 1               Gate 

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Rec. Fwd.    Configuration Pages   Mode AStart Quantize                    
  // G1T1 off   on  Step>Live<Ptn. MIDI Misc Poly  on       10%                      

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Rec. Fwd.    Configuration Pages   Drum Ptn.   Pattern  1    Length        
  // G1T1 off   on  Step Live>Ptn.<MIDI Misc  BD   on  *.*.*.*.*.*.*.*.  75% CpyPaste

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. G1T1   Pattern  1  *.*.*.*.*.*.*.*.                              Cfg.  Ptn.
  //  > 1<   2    3    4    5    6    7    8    9  10   11   12   13   14  Page   on 


  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Rec. Fwd.    Configuration Pages    Bus Port Chn. Lower/Upper Mode   Reset 
  // G1T1 off   on  Step Live Ptn.>MIDI<Misc   1  IN1  #16   ---   ---  Jam    Stacks

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Rec. Fwd.    Configuration Pages   Oct.  Fx   FTS                          
  // G1T1 off   on  Step Live Ptn. MIDI>Misc< +0   on    on                          


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 ix = (event_mode == SEQ_EVENT_MODE_Drum) ? (1+ui_selected_instrument) : 0;
  seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t *)&seq_live_pattern_slot[ix];    


  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  if( selected_subpage == SUBPAGE_PATSEL ) {
    u8 pattern_num = slot->pattern;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("Trk. ");
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintFormattedString("   Pattern %2d  ", pattern_num + 1);
    SEQ_UI_TRKJAM_Hlp_PrintPattern(slot->pattern);
    SEQ_LCD_PrintSpaces(30);
    SEQ_LCD_PrintString("Cfg.  Ptn.");

    SEQ_LCD_CursorSet(0, 1);
    {
      int i;
      for(i=0; i<14; ++i) {
	if( i == pattern_num ) {
	  SEQ_LCD_PrintFormattedString(">%2d< ", i+1);
	} else {
	  SEQ_LCD_PrintFormattedString(" %2d  ", i+1);
	}
      }
    }

    SEQ_LCD_PrintString("Page  ");
    SEQ_LCD_PrintString(slot->enabled ? " on " : "off ");

    return 0; // no error
  }

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Rec. Fwd.    Configuration Pages   ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REC_ENABLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_state.ENABLED ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_FWD_ENABLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_options.FWD_MIDI ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString(" Step Live Ptn. MIDI Misc ");

  if( selected_subpage <= 5 ) {
    const u8 select_pos1[5]  = { 14, 19, 24, 29, 34 };
    const u8 select_width[5] = {  4,  4,  4,  4,  4 };

    SEQ_LCD_CursorSet(select_pos1[selected_subpage], 1);
    SEQ_LCD_PrintChar('>');
    if( ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(select_width[selected_subpage]);
    }
    SEQ_LCD_CursorSet(select_pos1[selected_subpage] + select_width[selected_subpage] + 1, 1);
    SEQ_LCD_PrintChar('<');
  }

  switch( selected_subpage ) {
  ///////////////////////////////////////////////////////////////////////////
  case SUBPAGE_REC_STEP: {
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintString("Mode AStart Step Inc.             Toggle");

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 1);
    if( ui_selected_item == ITEM_STEP_POLY_MODE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString(seq_record_options.POLY_RECORD ? "Poly" : "Mono");
      SEQ_LCD_PrintChar((seq_record_options.POLY_RECORD && GetNumNoteLayers() < 2) ? '!' : ' ');
    }
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_STEP_AUTO_START && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintString(seq_record_options.AUTO_START ? "on " : "off");
    }
    SEQ_LCD_PrintSpaces(3);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_STEP_RECORD_STEP && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintFormattedString("%3d", ui_selected_step+1);
    }
    SEQ_LCD_PrintSpaces(2);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_STEP_RECORD_INC && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      if( !seq_record_options.POLY_RECORD ) {
	SEQ_LCD_PrintFormattedString("+%2d", seq_record_options.STEPS_PER_KEY);
      } else {
	SEQ_LCD_PrintString("---");
      }
    }
    SEQ_LCD_PrintSpaces(15);

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_PrintString("Gate ");

  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SUBPAGE_REC_LIVE: {
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintString("Mode AStart Quantize                    ");

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 1);
    if( ui_selected_item == ITEM_LIVE_POLY_MODE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString(seq_record_options.POLY_RECORD ? "Poly" : "Mono");
      SEQ_LCD_PrintChar((seq_record_options.POLY_RECORD && GetNumNoteLayers() < 2) ? '!' : ' ');
    }
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LIVE_AUTO_START && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintString(seq_record_options.AUTO_START ? "on " : "off");
    }
    SEQ_LCD_PrintSpaces(5);

    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LIVE_QUANTIZE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintFormattedString("%3d%%", seq_record_quantize);
    }
    SEQ_LCD_PrintSpaces(22);
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SUBPAGE_REC_PTN: {
    // using drum symbols
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsBig);

    SEQ_LCD_CursorSet(40, 0);
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintString("Drum");
    } else {
      SEQ_LCD_PrintString("Mode");
    }

    SEQ_LCD_PrintFormattedString(" Ptn.   Pattern %2d    Length ", slot->pattern + 1);

    if( seq_ui_button_state.SELECT_PRESSED ) {
      SEQ_LCD_PrintString("   EDIT");
    } else {
      SEQ_LCD_PrintSpaces(7);
    }

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 1);
    if( ui_selected_item == ITEM_PTN_POLY_OR_DRUM && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
      } else {
	SEQ_LCD_PrintString(seq_record_options.POLY_RECORD ? "Poly" : "Mono");
	SEQ_LCD_PrintChar((seq_record_options.POLY_RECORD && GetNumNoteLayers() < 2) ? '!' : ' ');
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_PTN_ENABLE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString(slot->enabled ? " on" : "off");
      SEQ_LCD_PrintSpaces(2);
    }

    ///////////////////////////////////////////////////////////////////////////
    SEQ_UI_TRKJAM_Hlp_PrintPattern(slot->pattern);
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_PTN_LENGTH && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintGatelength(slot->len + 1);
    }

    SEQ_LCD_PrintString(" CpyPaste");

  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SUBPAGE_MIDI: {
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintString(" Bus Port Chn. Lower/Upper Mode   Reset ");

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 1);
    if( ui_selected_item == ITEM_MIDI_IN_BUS && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintFormattedString("  %d  ", selected_bus+1);
    }

    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_IN_PORT && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      if( seq_midi_in_port[selected_bus] )
	SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus])));
      else
	SEQ_LCD_PrintString(" All");
    }
    SEQ_LCD_PrintSpaces(1);


    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_IN_CHN && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      if( seq_midi_in_channel[selected_bus] == 17 )
	SEQ_LCD_PrintString("All");
      else if( seq_midi_in_channel[selected_bus] )
	SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_channel[selected_bus]);
      else
	SEQ_LCD_PrintString("---");
    }
    SEQ_LCD_PrintSpaces(3);


    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_IN_LOWER && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintNote(seq_midi_in_lower[selected_bus]);
    }
    SEQ_LCD_PrintSpaces(3);


    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_IN_UPPER && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintNote(seq_midi_in_upper[selected_bus]);
    }
    SEQ_LCD_PrintSpaces(2);


    ///////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MIDI_IN_MODE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintString(seq_midi_in_options[selected_bus].MODE_PLAY ? "Jam " : "T&A ");
    }
    SEQ_LCD_PrintSpaces(3);

    SEQ_LCD_PrintString("Stacks");
  } break;

  ///////////////////////////////////////////////////////////////////////////
  case SUBPAGE_MISC: {
    SEQ_LCD_CursorSet(40, 0);
    SEQ_LCD_PrintString("Oct.  Fx   FTS                          ");

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(40, 1);
    if( ui_selected_item == ITEM_MISC_OCT_TRANSPOSE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintString("Drum");
      } else {
	SEQ_LCD_PrintFormattedString(" %c%d ", (seq_live_options.OCT_TRANSPOSE < 0) ? '-' : '+', abs(seq_live_options.OCT_TRANSPOSE));
      }
    }
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MISC_FX_ENABLE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString(seq_live_options.FX ? " on  " : " off ");
    }

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MISC_FTS && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString(seq_live_options.FORCE_SCALE ? "  on " : " off ");
    }

    SEQ_LCD_PrintSpaces(30);
  } break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( ui_store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    ui_store_file_required = 0;
  }

  // disable recording
  // SEQ_RECORD_Enable(0, 1);
  // not in revised trkjam page anymore

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKJAM_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  // enable recording
  // SEQ_RECORD_Enable(1, 1);
  // no auto-enable in revised page anymore...

  // switch step recording depending on subpage
  seq_record_options.STEP_RECORD = (selected_subpage == SUBPAGE_REC_STEP) ? 1 : 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns != 0 if poly mode possible with currently selected track
/////////////////////////////////////////////////////////////////////////////
static s32 GetNumNoteLayers(void)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  u8 *layer_type_ptr = (u8 *)&seq_cc_trk[visible_track].lay_const[0*16];
  u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
  int par_layer;
  int note_layers = 0;
  for(par_layer=0; par_layer<num_p_layers; ++par_layer, ++layer_type_ptr) {
      if( *layer_type_ptr == SEQ_PAR_Type_Note )
	++note_layers;
  }

  return note_layers;
}


/////////////////////////////////////////////////////////////////////////////
// For copy/paste/clear button in seq_ui.c:
// Returns 1 if pattern subpage is selected
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKJAM_PatternRecordSelected(void)
{
  return (ui_page == SEQ_UI_PAGE_TRKJAM && selected_subpage == SUBPAGE_REC_PTN) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// Prints selected Live Pattern (16 chars)
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_TRKJAM_Hlp_PrintPattern(u8 pattern_num)
{
  seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[pattern_num];
  int step;
  u16 mask = 0x0001;
  for(step=0; step<16; ++step, mask <<= 1) {
    u8 gate = (pattern->gate & mask) ? 1 : 0;
    u8 accent = (pattern->accent & mask) ? 2 : 0;
    SEQ_LCD_PrintChar(gate | accent);
  }

  return 0; // no error
}
