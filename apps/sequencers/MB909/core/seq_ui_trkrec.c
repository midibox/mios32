// $Id: seq_ui_trkrec.c 1341 2011-10-16 21:06:08Z tk $
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
#include "seq_file.h"
#include "seq_file_c.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS      10
#define ITEM_GXTY          0
#define ITEM_STEP_MODE     1
#define ITEM_POLY_MODE     2
#define ITEM_AUTO_START    3
#define ITEM_RECORD_STEP   4
#define ITEM_TOGGLE_GATE   5
#define ITEM_REC_PORT      6
#define ITEM_REC_CHN       7
#define ITEM_FWD_MIDI      8
#define ITEM_QUANTIZE      9


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 store_file_required;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 GetNumNoteLayers(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  // branch to edit page if new event has been recorded (ui_hold_msg_ctr controlled from SEQ_RECORD_*)
  if( ui_hold_msg_ctr )
    return SEQ_UI_EDIT_LED_Handler(gp_leds);

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_STEP_MODE: *gp_leds = 0x0002; break;
    case ITEM_POLY_MODE: *gp_leds = 0x0004; break;
    case ITEM_AUTO_START: *gp_leds = 0x0018; break;
    case ITEM_RECORD_STEP: *gp_leds = 0x0020; break;
    case ITEM_TOGGLE_GATE: *gp_leds = 0x00c0; break;
    case ITEM_REC_PORT: *gp_leds = 0x0100; break;
    case ITEM_REC_CHN: *gp_leds = 0x0200; break;
    case ITEM_FWD_MIDI: *gp_leds = 0x0c00; break;
    case ITEM_QUANTIZE: *gp_leds = 0x3000; break;
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

  // ensure that original record screen will be print immediately
  ui_hold_msg_ctr = 0;

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_STEP_MODE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_POLY_MODE;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_AUTO_START;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_RECORD_STEP;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_TOGGLE_GATE;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_REC_PORT;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_REC_CHN;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_FWD_MIDI;
      break;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_QUANTIZE;
      break;

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return 0; // not mapped yet
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:
      return SEQ_UI_GxTyInc(incrementer);

    case ITEM_STEP_MODE:
      if( incrementer ) {
	u8 tmp;
	if( seq_record_options.STEP_RECORD )
	  tmp = seq_record_options.STEPS_PER_KEY + 1;
	else
	  tmp = 0;

	if( SEQ_UI_Var8_Inc(&tmp, 0, 16+1, incrementer) > 0 ) {
	  if( tmp ) {
	    seq_record_options.STEPS_PER_KEY = tmp - 1;
	    seq_record_options.STEP_RECORD = 1; // Live->Step Record Mode
	  } else {
	    seq_record_options.STEPS_PER_KEY = 0;
	    seq_record_options.STEP_RECORD = 0; // Step->Live Record Mode
	  }
	} else
	  return 0; // no change
      } else
	seq_record_options.STEP_RECORD ^= 1;
      return 1;

    case ITEM_POLY_MODE:
      if( incrementer )
	seq_record_options.POLY_RECORD = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.POLY_RECORD ^= 1;

        // print warning if poly mode not possible
	if( seq_record_options.POLY_RECORD ) {
	  int num_note_layers = GetNumNoteLayers();
	  if( num_note_layers < 2 ) {
	    SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "No Poly Recording:",
		       num_note_layers ? "Track has only one Note Layer!" : "Track has no Note Layers!");
	  }
	}
      return 1;

    case ITEM_AUTO_START:
      if( incrementer )
	seq_record_options.AUTO_START = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.AUTO_START ^= 1;
      return 1;

    case ITEM_RECORD_STEP: {
      u8 step = seq_record_step;
      if( SEQ_UI_Var8_Inc(&step, 0, SEQ_CC_Get(visible_track, SEQ_CC_LENGTH), incrementer) > 0 ) {
	u8 track;
	for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
	  SEQ_RECORD_Reset(track);

	seq_record_step = step;

	// print edit screen
	SEQ_RECORD_PrintEditScreen();

	return 1;
      }
      return 0;
    }

    case ITEM_TOGGLE_GATE: {
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
	gate = SEQ_TRG_GateGet(visible_track, seq_record_step, ui_selected_instrument) ? 0 : 1;

      int i;
      for(i=0; i<SEQ_TRG_NumInstrumentsGet(visible_track); ++i)
	SEQ_TRG_GateSet(visible_track, seq_record_step, i, gate);

      // increment step
      int next_step = (seq_record_step + seq_record_options.STEPS_PER_KEY) % ((int)SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1);

      for(i=0; i<SEQ_CORE_NUM_TRACKS; ++i)
	SEQ_RECORD_Reset(i);

      seq_record_step = next_step;

      // print edit screen
      SEQ_RECORD_PrintEditScreen();

      return 1;
    }

    case ITEM_REC_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_rec_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1, incrementer) >= 0 ) {
	seq_midi_in_rec_port = SEQ_MIDI_PORT_InPortGet(port_ix);
	store_file_required = 1;
	SEQ_RECORD_AllNotesOff(); // reset note markers
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_REC_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_rec_channel, 0, 16, incrementer) >= 0 ) {
	store_file_required = 1;
	SEQ_RECORD_AllNotesOff(); // reset note markers
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_FWD_MIDI:
      if( incrementer )
	seq_record_options.FWD_MIDI = incrementer > 0 ? 1 : 0;
      else
	seq_record_options.FWD_MIDI ^= 1;
      return 1;

    case ITEM_QUANTIZE:
      if( SEQ_UI_Var8_Inc(&seq_record_quantize, 0, 99, incrementer) >= 0 ) {
	return 1; // value changed
      }
      return 0; // no change
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
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
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
  // Trk. Record Mode  AStart  Step  TglGate Port Chn.  Fwd.Midi Quantize            
  // G1T1 Live   Poly    on     16           IN1  # 1      on       20%

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Record Mode");  
  SEQ_LCD_CursorSet(0, 2);
  SEQ_LCD_PrintString("	  AStart  Step  TglGate");
  SEQ_LCD_CursorSet(0, 4);
  SEQ_LCD_PrintString("	  Port Chn.  Fwd.Midi ");
  SEQ_LCD_CursorSet(0, 6);
  SEQ_LCD_PrintString("	  Quantize            ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_STEP_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    if( seq_record_options.STEP_RECORD ) {
      if( seq_record_options.STEPS_PER_KEY < 10 )
	SEQ_LCD_PrintFormattedString("Step+%d", seq_record_options.STEPS_PER_KEY);
      else
	SEQ_LCD_PrintFormattedString("Step%d", seq_record_options.STEPS_PER_KEY);
    } else {
      SEQ_LCD_PrintString(" Live ");
    }
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_POLY_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintString(seq_record_options.POLY_RECORD ? "Poly" : "Mono");
    SEQ_LCD_PrintChar((seq_record_options.POLY_RECORD && GetNumNoteLayers() < 2) ? '!' : ' ');
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 3);
  if( ui_selected_item == ITEM_AUTO_START && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_options.AUTO_START ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RECORD_STEP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", seq_record_step+1);
  }
  SEQ_LCD_PrintSpaces(11);

  ///////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 5);
  if( ui_selected_item == ITEM_REC_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( seq_midi_in_rec_port )
      SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_rec_port)));
    else
      SEQ_LCD_PrintString(" All");
  }
  SEQ_LCD_PrintSpaces(1);


  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REC_CHN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( seq_midi_in_rec_channel )
      SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_rec_channel);
    else
      SEQ_LCD_PrintString("---");
  }
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_FWD_MIDI && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_options.FWD_MIDI ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(6);

  ///////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 7);
  if( ui_selected_item == ITEM_QUANTIZE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d%%", seq_record_quantize);
  }
  SEQ_LCD_PrintSpaces(14);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;
  }

  // disable recording
  seq_record_state.ENABLED = 0;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKREC_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  store_file_required = 0;

  // enable recording
  seq_record_state.ENABLED = 1;

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
