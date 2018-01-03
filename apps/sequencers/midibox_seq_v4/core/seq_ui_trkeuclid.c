// $Id$
/*
 * Euclidean Algorithm Generator
 * Inspired from:
 *   - Ruin & Wesen: http://ruinwesen.com/blog?id=216
 *   - crx: http://crx091081gb.net/?p=189
 *
 * ==========================================================================
 *
 *  Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
 *  Copyright of "Eugen" algorithm: crx
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

#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_cc.h"
#include "seq_random.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS         9
#define ITEM_GXTY            0
#define ITEM_TRK_LENGTH      1
#define ITEM_PARTRG_SELECT   2
#define ITEM_PAR_VALUE       3
#define ITEM_DRUM_VEL_N      4
#define ITEM_DRUM_VEL_A      5
#define ITEM_RND_ACC_PROB    6
#define ITEM_EUCLID_LENGTH   7
#define ITEM_EUCLID_PULSES   8
#define ITEM_EUCLID_OFFSET   9


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// pre-selections for each trigger layer separately
#define NUM_EUCLID_CFG 16
static u8 euclid_length[NUM_EUCLID_CFG] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static u8 euclid_pulses[NUM_EUCLID_CFG] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static u8 euclid_offset[NUM_EUCLID_CFG] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static u8 rnd_acc_probability = 50;
static u8 rnd_acc_n = 100; // only for non-drum layers
static u8 rnd_acc_a = 127; // only for non-drum layers
static u8 par_val[16] = { // used parameter layer values
  0x3c, 0x3c, 0x3c, 0x3c,
  0x3c, 0x3c, 0x3c, 0x3c,
  0x3c, 0x3c, 0x3c, 0x3c,
  0x3c, 0x3c, 0x3c, 0x3c,
};


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 EuclidGenerator(u8 track, u16 steps, u16 pulses, u16 offset);
static s32 AccentGenerator(u8 track, u16 steps);
static s32 ReGenAccent(u8 track, u8 prev_rnd_acc_n, u8 prev_rnd_acc_a);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY:          *gp_leds = 0x0001; break;
    case ITEM_TRK_LENGTH:    *gp_leds = 0x0006; break;
    case ITEM_PARTRG_SELECT: *gp_leds = 0x0008; break;
    case ITEM_PAR_VALUE:     *gp_leds = 0x0010; break;
    case ITEM_DRUM_VEL_N:    *gp_leds = 0x0020; break;
    case ITEM_DRUM_VEL_A:    *gp_leds = 0x0040; break;
    case ITEM_RND_ACC_PROB:  *gp_leds = 0x0080; break;
    case ITEM_EUCLID_LENGTH: *gp_leds = 0x0100; break;
    case ITEM_EUCLID_PULSES: *gp_leds = 0x0200; break;
    case ITEM_EUCLID_OFFSET: *gp_leds = 0x0400; break;
  }

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
  u8 euclid_layer = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : ui_selected_trg_layer;
  u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_TRK_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_PARTRG_SELECT;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_PAR_VALUE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_DRUM_VEL_N;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_DRUM_VEL_A;
      break;

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_RND_ACC_PROB;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_EUCLID_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_EUCLID_PULSES;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_EUCLID_OFFSET;
      break;

    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return 0; // no encoder function
  }

  switch( ui_selected_item ) {
  case ITEM_GXTY: return SEQ_UI_GxTyInc(incrementer);

  case ITEM_TRK_LENGTH: {    
    if( SEQ_UI_CC_Inc(SEQ_CC_LENGTH, 0, num_steps-1, incrementer) >= 1 ) {
      if( seq_cc_trk[visible_track].clkdiv.SYNCH_TO_MEASURE && 
	  (int)SEQ_CC_Get(visible_track, SEQ_CC_LENGTH) > (int)seq_core_steps_per_measure ) {
	char buffer[20];
	sprintf(buffer, "active for %d steps", (int)seq_core_steps_per_measure+1);
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Synch-to-Measure is", buffer);
      }
      return 1;
    }
  } break;

  case ITEM_PARTRG_SELECT: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      u8 num_drums = SEQ_TRG_NumInstrumentsGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_drums-1, incrementer);
    } else {
      u8 num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_par_layer, 0, num_p_layers-1, incrementer);
    }
  } break;

  case ITEM_PAR_VALUE: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      if( SEQ_UI_Var8_Inc(&par_val[ui_selected_par_layer], 0, 127, incrementer) > 0 ) {
	EuclidGenerator(visible_track, (int)euclid_length[euclid_layer]+1, euclid_pulses[euclid_layer], euclid_offset[euclid_layer]);
	return 1;
      }
      return 0;
    }
  } break;

  case ITEM_DRUM_VEL_N: {
    if( event_mode == SEQ_EVENT_MODE_Drum && !SEQ_CC_TrackHasVelocityParLayer(visible_track) ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      u8 prev_rnd_acc_n = rnd_acc_n;
      if( SEQ_UI_Var8_Inc(&rnd_acc_n, 1, rnd_acc_a-1, incrementer) >= 1 ) {
	// re-generate accent
	ReGenAccent(visible_track, prev_rnd_acc_n, rnd_acc_a);
      }
      return 1;
    }
  } break;

  case ITEM_RND_ACC_PROB: {
    incrementer *= 10; // more comfortable to do this in bigger steps...
    SEQ_UI_Var8_Inc(&rnd_acc_probability, 0, 100, incrementer);

    // generate new pattern
    AccentGenerator(visible_track, (int)euclid_length[euclid_layer]+1);

    return 1;
  } break;

  case ITEM_DRUM_VEL_A: {
    if( event_mode == SEQ_EVENT_MODE_Drum && !SEQ_CC_TrackHasVelocityParLayer(visible_track) ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_C1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      u8 prev_rnd_acc_a = rnd_acc_a;
      if( SEQ_UI_Var8_Inc(&rnd_acc_a, rnd_acc_n+1, 127, incrementer) >= 1 ) {
	// re-generate accent
	ReGenAccent(visible_track, rnd_acc_n, prev_rnd_acc_a);
      }
      return 1;
    }
  } break;

  case ITEM_EUCLID_LENGTH:
  case ITEM_EUCLID_OFFSET:
  case ITEM_EUCLID_PULSES: {

    if( ui_selected_item == ITEM_EUCLID_LENGTH ) {
      SEQ_UI_Var8_Inc((u8 *)&euclid_length[euclid_layer], 0, num_steps-1, incrementer);
    } else if( ui_selected_item == ITEM_EUCLID_OFFSET ) {
      SEQ_UI_Var8_Inc((u8 *)&euclid_offset[euclid_layer], 0, (num_steps > 255) ? 255 : num_steps, incrementer);
    } else {
      SEQ_UI_Var8_Inc((u8 *)&euclid_pulses[euclid_layer], 0, (num_steps > 255) ? 255 : num_steps, incrementer);
    }

    // generate new pattern
    EuclidGenerator(visible_track, (int)euclid_length[euclid_layer]+1, euclid_pulses[euclid_layer], euclid_offset[euclid_layer]);

    // +accent if drum track or first par layer
    if( event_mode == SEQ_EVENT_MODE_Drum || ui_selected_par_layer == 0 ) {
      AccentGenerator(visible_track, (int)euclid_length[euclid_layer]+1);
    }

    return 1;
  } break;

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

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( depressed ) return -1;
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      if( depressed ) return -1;
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      if( depressed ) return -1;
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. TrkLength Drum Note VelN VelA RndA Len Pulses Offs.    *.**.**.**.**.*.*.*.
  // G3T2   64/128   CH   F#1  100  127  50%  20   12     0                          

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. TrkLength ParA Val. VelN VelA RndA Len Pulses Offs.    *..*..*..*..*.*.*..*
  // G1T1   64/128  Note Orig  100  127  50%  16    6     0                          

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 euclid_layer = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : ui_selected_trg_layer;


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. TrkLength ");
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintString("Drum Note VelN VelA RndA ");
  } else {
    SEQ_LCD_PrintFormattedString("Par%c", 'A'+ui_selected_par_layer);
    SEQ_LCD_PrintString(" Val. VelN VelA RndA ");
  }
  SEQ_LCD_PrintString("Len Pulses Offs.    ");

  ///////////////////////////////////////////////////////////////////////////
  {
    u16 len = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1;
    u8 max_trigger = (len > 20) ? 20 : len;
    seq_cc_trk_t *tcc = &seq_cc_trk[visible_track];    

    int i;
    for(i=0; i<max_trigger; ++i) {
      u8 gate = SEQ_TRG_GateGet(visible_track, i, ui_selected_instrument);
      u8 gate_disabled = 0;
      if( event_mode != SEQ_EVENT_MODE_Drum && ui_selected_par_layer > 0 ) {
	if( !gate ) {
	  gate_disabled = SEQ_PAR_Get(visible_track, i, ui_selected_par_layer, ui_selected_instrument) > 0;
	} else {
	  gate = SEQ_PAR_Get(visible_track, i, ui_selected_par_layer, ui_selected_instrument) > 0;
	}
      }

      if( gate_disabled ) {
	SEQ_LCD_PrintChar('-');
      } else {
	u8 accent = !gate ? 0 : SEQ_TRG_AccentGet(visible_track, i, ui_selected_instrument);

	if( gate && !accent && tcc->link_par_layer_velocity >= 0 ) {
	  u8 value = SEQ_PAR_Get(visible_track, i, tcc->link_par_layer_velocity, ui_selected_instrument);
	  if( rnd_acc_n <= rnd_acc_a ) {
	    if( value >= rnd_acc_a )
	      accent |= 1;
	  } else {
	    if( value <= rnd_acc_a )
	      accent |= 1;
	  }
	}
	SEQ_LCD_PrintChar(gate | (accent << 1));
      }
    }

    for(;i<20; ++i) {
      SEQ_LCD_PrintChar(' ');
    }
  }
  

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_TRK_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(10);
  } else {
    u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
    u16 len = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1;

    if( len > num_steps )
      SEQ_LCD_PrintFormattedString(" !!!/%3d  ", num_steps);
    else
      SEQ_LCD_PrintFormattedString(" %3d/%3d  ", len, num_steps);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PARTRG_SELECT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
    } else {
      char str_buffer[6];
      SEQ_PAR_AssignedTypeStr(visible_track, ui_selected_par_layer, ui_selected_instrument, str_buffer);
      SEQ_LCD_PrintString(str_buffer);
    }
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PAR_VALUE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintSpaces(1);
      SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument));
      SEQ_LCD_PrintSpaces(1);
    } else {
      if( ui_selected_par_layer == 0 ) {
	SEQ_LCD_PrintString("Orig ");
      } else {
	SEQ_LCD_PrintLayerValue(visible_track, ui_selected_par_layer, par_val[ui_selected_par_layer]);
	SEQ_LCD_PrintSpaces(1);
      }
    }
  }
  SEQ_LCD_PrintSpaces(1);

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_DRUM_VEL_N && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintFormattedString("%3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_instrument));
    } else {
      SEQ_LCD_PrintFormattedString("%3d ", rnd_acc_n);
    }
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_DRUM_VEL_A && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_C1 + ui_selected_instrument));
    } else {
      SEQ_LCD_PrintFormattedString(" %3d ", rnd_acc_a);
    }
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RND_ACC_PROB && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%% ", rnd_acc_probability);
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EUCLID_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", (int)euclid_length[euclid_layer]+1);
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EUCLID_PULSES && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintFormattedString(" %3d  ", euclid_pulses[euclid_layer]);
  }

  /////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_EUCLID_OFFSET && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(10);
  } else {
    SEQ_LCD_PrintFormattedString(" %3d      ", euclid_offset[euclid_layer]);
  }

  /////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(30);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKEUCLID_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsBig);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// The Euclid Generator
// Algorithm from crx (http://crx091081gb.net/?p=189)
/////////////////////////////////////////////////////////////////////////////
static s32 EuclidGenerator(u8 track, u16 steps, u16 pulses, u16 offset)
{
  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  u8 instrument = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;
  u8 modify_par_layer = event_mode != SEQ_EVENT_MODE_Drum && ui_selected_par_layer > 0;


  // to simplify code
#define EUCLID_SET_GATE(xstep, xgate) if( modify_par_layer ) {		\
                                        SEQ_PAR_Set(track, xstep, ui_selected_par_layer, instrument, (xgate) ? par_val[ui_selected_par_layer] : 0); \
                                      } else { \
                                        SEQ_TRG_GateSet(track, xstep, instrument, xgate); \
                                      }

  if( pulses >= steps ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      EUCLID_SET_GATE(step, 1);
    }
  } else if( pulses == 0 ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      EUCLID_SET_GATE(step, 0);
    }
  } else if( steps == 1 ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      EUCLID_SET_GATE(step, (pulses && step == offset) ? 1 : 0);
    }
  } else {
    int pauses = steps - pulses;

    if( pauses >= pulses ) { // first case: more pauses than pulses
      int per_pulse = pauses / pulses;
      int remainder = pauses % pulses;

      int loop_offset; // fill whole track with repeating pattern
      for(loop_offset=0; loop_offset<num_steps; loop_offset += steps) {
	u16 step = offset;
	u16 processed_steps = 0;

	int i;
	for(i=0; i<pulses; ++i) {
	  EUCLID_SET_GATE(loop_offset + step, 1);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;
	  
	  int j;
	  for(j=0; j<per_pulse; ++j) {
	    EUCLID_SET_GATE(loop_offset + step, 0);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;

	  if( i < remainder ) {
	    EUCLID_SET_GATE(loop_offset + step, 0);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;
	}
      }
    } else { // second case: more pulses than pauses
      int per_pause = (pulses-pauses) / pauses;
      int remainder = (pulses-pauses) % pauses;

      int loop_offset; // fill whole track with repeating pattern
      for(loop_offset=0; loop_offset<num_steps; loop_offset += steps) {
	u16 step = offset;
	u16 processed_steps = 0;

	int i;
	for(i=0; i<pauses; ++i) {
	  EUCLID_SET_GATE(loop_offset + step, 1);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;

	  EUCLID_SET_GATE(loop_offset + step, 0);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;

	  int j;
	  for(j=0; j<per_pause; ++j) {
	    EUCLID_SET_GATE(loop_offset + step, 1);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;

	  if( i < remainder ) {
	    EUCLID_SET_GATE(loop_offset + step, 1);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;
	}
      }
    }
  }

  SEQ_CORE_CancelSustainedNotes(track);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Random Accent Generator
/////////////////////////////////////////////////////////////////////////////
static s32 AccentGenerator(u8 track, u16 steps)
{
  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  u8 instrument = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;

  int loop_offset; // fill whole track with repeating pattern
  seq_cc_trk_t *tcc = &seq_cc_trk[track];    

  for(loop_offset=0; loop_offset<num_steps; loop_offset += steps) {
    u8 step;
    for(step=0; step<steps; ++step) {
      if( loop_offset == 0 ) {
	// generate random accent
	u8 rnd = SEQ_RANDOM_Gen_Range(0, 100);
	u8 accent = rnd < rnd_acc_probability;

	if( tcc->link_par_layer_velocity >= 0 ) {
	  SEQ_PAR_Set(track, step, tcc->link_par_layer_velocity, instrument,
		      accent ? rnd_acc_a : rnd_acc_n);
	} else {
	  SEQ_TRG_AccentSet(track, step, instrument, accent);
	}
      } else {
	// copy first loop to remaining loop ranges
	if( tcc->link_par_layer_velocity >= 0 ) {
	  SEQ_PAR_Set(track, loop_offset+step, tcc->link_par_layer_velocity, instrument,
		      SEQ_PAR_Get(track, step, tcc->link_par_layer_velocity, instrument));
	} else {
	  SEQ_TRG_AccentSet(track, loop_offset+step, instrument,
			    SEQ_TRG_AccentGet(track, step, instrument));
	}
      }
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Call this function whenever the velocity value has been changed
/////////////////////////////////////////////////////////////////////////////
static s32 ReGenAccent(u8 track, u8 prev_rnd_acc_n, u8 prev_rnd_acc_a)
{
  seq_cc_trk_t *tcc = &seq_cc_trk[track];    
  if( tcc->link_par_layer_velocity < 0 )
    return 0; // no need to re-generate

  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  u16 num_steps = SEQ_TRG_NumStepsGet(track);
  u8 instrument = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;

  {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      // was step accented?
      u8 vel = SEQ_PAR_Get(track, step, tcc->link_par_layer_velocity, instrument);
      u8 accent = (vel >= prev_rnd_acc_a) && (vel != prev_rnd_acc_n);

      SEQ_PAR_Set(track, step, tcc->link_par_layer_velocity, instrument,
		  accent ? rnd_acc_a : rnd_acc_n);
    }
  }

  return 0; // no error
}
