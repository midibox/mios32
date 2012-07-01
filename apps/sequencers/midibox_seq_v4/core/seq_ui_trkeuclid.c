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
#define ITEM_TRG_SELECT      2
#define ITEM_DRUM_NOTE       3
#define ITEM_DRUM_VEL_N      4
#define ITEM_DRUM_VEL_A      5
#define ITEM_EUCLID_LENGTH   6
#define ITEM_EUCLID_PULSES   7
#define ITEM_EUCLID_OFFSET   8


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// pre-selections for each trigger layer separately
#define NUM_EUCLID_CFG 16
static u8 euclid_length[NUM_EUCLID_CFG] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static u8 euclid_pulses[NUM_EUCLID_CFG] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
static u8 euclid_offset[NUM_EUCLID_CFG] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 EuclidGenerator(u8 track, u16 steps, u16 pulses, u16 offset);


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
    case ITEM_TRG_SELECT:    *gp_leds = 0x0008; break;
    case ITEM_DRUM_NOTE:     *gp_leds = 0x0010; break;
    case ITEM_DRUM_VEL_N:    *gp_leds = 0x0020; break;
    case ITEM_DRUM_VEL_A:    *gp_leds = 0x0040; break;
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
      ui_selected_item = ITEM_TRG_SELECT;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_DRUM_NOTE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_DRUM_VEL_N;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_DRUM_VEL_A;
      break;

    case SEQ_UI_ENCODER_GP8:
      return 0; // no encoder function

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

  case ITEM_TRG_SELECT: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      u8 num_drums = SEQ_TRG_NumInstrumentsGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_drums-1, incrementer);
    } else {
      u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_trg_layer, 0, num_t_layers-1, incrementer);
    }
  } break;

  case ITEM_DRUM_NOTE: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_A1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      return -1; // not supported
    }
  } break;

  case ITEM_DRUM_VEL_N: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      return -1; // not supported
    }
  } break;

  case ITEM_DRUM_VEL_A: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      return SEQ_UI_CC_Inc(SEQ_CC_LAY_CONST_B1 + ui_selected_instrument, 0, 127, incrementer);
    } else {
      return -1; // not supported
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
  // Trk. TrkLength Drum Note VelN VelA      Len Pulses Offs.    *.**.**.**.**.*.*.*.
  // G3T2   64/128   CH   F#1  100  127       20   12     0                          

  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. TrkLength Trg.                     Len Pulses Offs.    *..*..*..*..*.*.*..*
  // G1T1   64/128  Gate                      16    6     0                          

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 euclid_layer = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : ui_selected_trg_layer;


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. TrkLength ");
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintString("Drum Note VelN VelA      ");
  } else {
    SEQ_LCD_PrintString("Trg.                     ");
  }
  SEQ_LCD_PrintString("Len Pulses Offs.    ");

  ///////////////////////////////////////////////////////////////////////////
  {
    u16 len = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1;
    u8 max_trigger = (len > 20) ? 20 : len;

    int i;
    for(i=0; i<max_trigger; ++i) {
      u8 gate_accent = SEQ_TRG_GateGet(visible_track, i, ui_selected_instrument);
      gate_accent |= SEQ_TRG_AccentGet(visible_track, i, ui_selected_instrument) << 1;
      SEQ_LCD_PrintChar(gate_accent);
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
  if( ui_selected_item == ITEM_TRG_SELECT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
    } else {
      SEQ_LCD_PrintString(SEQ_TRG_AssignedTypeStr(visible_track, ui_selected_trg_layer));
    }
  }

  /////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(1);
  if( event_mode != SEQ_EVENT_MODE_Drum || (ui_selected_item == ITEM_DRUM_NOTE && ui_cursor_flash) ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_A1 + ui_selected_instrument));
  }
  SEQ_LCD_PrintSpaces(1);

  /////////////////////////////////////////////////////////////////////////
  if( event_mode != SEQ_EVENT_MODE_Drum || (ui_selected_item == ITEM_DRUM_VEL_N && ui_cursor_flash) ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_B1 + ui_selected_instrument));
  }

  /////////////////////////////////////////////////////////////////////////
  if( event_mode != SEQ_EVENT_MODE_Drum || (ui_selected_item == ITEM_DRUM_VEL_A && ui_cursor_flash) ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString(" %3d ", SEQ_CC_Get(visible_track, SEQ_CC_LAY_CONST_C1 + ui_selected_instrument));
  }

  /////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(6);

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

  if( pulses >= steps ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      SEQ_TRG_GateSet(track, step, instrument, 1);
    }
  } else if( pulses == 0 ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      SEQ_TRG_GateSet(track, step, instrument, 0);
    }
  } else if( steps == 1 ) {
    u16 step;
    for(step=0; step<num_steps; ++step) {
      SEQ_TRG_GateSet(track, step, instrument, (pulses && step == offset) ? 1 : 0);
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
	  SEQ_TRG_GateSet(track, loop_offset + step, instrument, 1);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;
	  
	  int j;
	  for(j=0; j<per_pulse; ++j) {
	    SEQ_TRG_GateSet(track, loop_offset + step, instrument, 0);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;

	  if( i < remainder ) {
	    SEQ_TRG_GateSet(track, loop_offset + step, instrument, 0);
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
	  SEQ_TRG_GateSet(track, loop_offset + step, instrument, 1);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;

	  SEQ_TRG_GateSet(track, loop_offset + step, instrument, 0);
	  if( ++processed_steps >= steps )
	    break;
	  step = (step + 1) % steps;

	  int j;
	  for(j=0; j<per_pause; ++j) {
	    SEQ_TRG_GateSet(track, loop_offset + step, instrument, 1);
	    if( ++processed_steps >= steps )
	      break;
	    step = (step + 1) % steps;
	  }

	  if( processed_steps >= steps )
	    break;

	  if( i < remainder ) {
	    SEQ_TRG_GateSet(track, loop_offset + step, instrument, 1);
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


  return 0; // no error
}
