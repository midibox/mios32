// $Id: seq_ui_stepsel.c 1751 2013-04-16 19:55:51Z tk $
/*
 * Step selection page (entered with Step View button)
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
#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_bpm.h"
#include "seq_core.h"
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_trg.h"
#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  int num_steps = SEQ_TRG_NumStepsGet(visible_track);

  if( num_steps > 128 )
    *gp_leds = 1 << ui_selected_step_view;
  else if( num_steps > 64 )
    *gp_leds = 3 << (2*ui_selected_step_view);
  else
    *gp_leds = 15 << (4*ui_selected_step_view);

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
  int num_steps = SEQ_TRG_NumStepsGet(visible_track);
  int track_length = (int)SEQ_CC_Get(visible_track, SEQ_CC_LENGTH) + 1;

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    if( seq_ui_button_state.SELECT_PRESSED ) {
      int section;
      if( num_steps > 128 )
	section = encoder / (track_length / 16);
      else if( num_steps > 64 )
	section = encoder / ((2 * track_length) / 16);
      else
	section = encoder / ((4 * track_length) / 16);

      // operation should be atomic (change all selected tracks)
      u8 track;
      seq_core_trk_t *t = &seq_core_trk[0];
      MIOS32_IRQ_Disable();
      for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track, ++t)
	if( ui_selected_tracks & (1 << track) )
	  t->play_section = section;
      MIOS32_IRQ_Enable();
    } else {
      // won't work if Follow function active!
      if( seq_core_state.FOLLOW ) {
	// print message and exit
	SEQ_UI_Msg((encoder <= SEQ_UI_ENCODER_GP8) ? SEQ_UI_MSG_USER : SEQ_UI_MSG_USER_R, 2000, "\"Follow\" active!", "Please deactivate!");
	return 1;
      }
    }

    // select new step view
    ui_selected_step_view = (encoder * (num_steps/16)) / 16;

    // select step within view
    if( !seq_ui_button_state.CHANGE_ALL_STEPS ) { // don't change the selected step if ALL function is active, otherwise the ramp can't be changed over multiple views
      ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);
    }

    if( seq_hwcfg_button_beh.step_view ) {
      // if toggle function active: jump back to previous menu
      // this is especially useful for the emulated MBSEQ, where we can only click on a single button
      // (stepview gets deactivated when clicking on GP button)
      if( seq_ui_button_state.STEP_VIEW ) {
	seq_ui_button_state.STEP_VIEW = 0;
	SEQ_UI_PageSet(ui_stepview_prev_page);
      }
    }

    return 1; // value changed
  } else if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    if( SEQ_UI_Var8_Inc(&ui_selected_step_view, 0, (num_steps-1)/16, incrementer) >= 1 ) {
      // select step within view
      if( !seq_ui_button_state.CHANGE_ALL_STEPS ) { // don't change the selected step if ALL function is active, otherwise the ramp can't be changed over multiple views
	ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);
      }
    } else {
      return 0;
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
    // -> same handling like for encoders
    return Encoder_Handler(button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
      return 1; // selects section mode, checked via seq_ui_button_state.SELECT_PRESSED

    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      if( depressed ) return 0; // ignore when button depressed
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
    case SEQ_UI_BUTTON_Down:
      if( depressed ) return 0; // ignore when button depressed
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
  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // 1                   17                  33                  49
  // *... *... *... *... *... *... *... *... *... *... *... *... *... *... *... *... 


  // 5 character usage in 256 step view (5 chars have to display 16 steps, 8 special chars available,
  // due to this limitation, we only display 15 steps (shouldn't really hurt))
  // 43210 43210 43210 43210 43210
  // * * * * . . . * . . . * . . .  (Small charset)

  // 5 character usage in 128 step view (5 chars have to display 8 steps):
  // 43210 43210 43210 43210 43210
  //  * *   . *   * .   . .         (Medium charset)

  // 5 character usage in 64 step view (5 chars have to display 4 steps):
  // 43210 43210 43210 43210 43210
  //   *     *     *     *     *    (Big charset)

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  int num_steps = SEQ_TRG_NumStepsGet(visible_track);
  int steps_per_item = num_steps / 16;
  int played_step = SEQ_BPM_IsRunning() ? seq_core_trk[visible_track].step : -1;

  int i;

  if( !high_prio ) {
    SEQ_LCD_CursorSet(0, 0);
    for(i=0; i<16; ++i){
      switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 0);
			break;
			case 4:
				SEQ_LCD_CursorSet(0, 2);
			break;
			case 8:
				SEQ_LCD_CursorSet(0, 4);
			break;
			case 12:
				SEQ_LCD_CursorSet(0, 6);
			break;
		}
	  if( ((i*steps_per_item) % 16) || ((((i*steps_per_item)/16) == ui_selected_step_view) && ui_cursor_flash) )
	SEQ_LCD_PrintSpaces(5);
      else
	SEQ_LCD_PrintFormattedString("%-3d  ", i*steps_per_item+1);
	}
    // print flashing *LOOPED* at right corner if loop mode activated to remind that steps will be played differntly
    if( (ui_cursor_flash_overrun_ctr & 1) && seq_core_state.LOOP ) {
      SEQ_LCD_CursorSet(11, 7);
      SEQ_LCD_PrintString(" *LOOPED*");
    } else if( (ui_cursor_flash_overrun_ctr & 1) && seq_core_trk[visible_track].play_section > 0 ) {
      SEQ_LCD_CursorSet(11, 7);
      SEQ_LCD_PrintFormattedString(" *Sect.%c*", 'A'+seq_core_trk[visible_track].play_section);
    } else {
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	// print drum name at the rightmost side
	SEQ_LCD_CursorSet(11, 7);
	SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
      } else {
	// print trigger layer and name at the rightmost side
	SEQ_LCD_CursorSet(13, 7);
	SEQ_LCD_PrintFormattedString("%c:%s", 'A' + ui_selected_trg_layer, SEQ_TRG_AssignedTypeStr(visible_track, ui_selected_trg_layer));
      }
    }
  }

  SEQ_LCD_CursorSet(0, 1);

  if( steps_per_item > 8 ) {
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsSmall);

    for(i=0; i<16; ++i) {
		switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 1);
			break;
			case 4:
				SEQ_LCD_CursorSet(0, 3);
			break;
			case 8:
				SEQ_LCD_CursorSet(0, 5);
			break;
			case 12:
				SEQ_LCD_CursorSet(0, 7);
			break;
		}
		
		
      u16 step = i*steps_per_item;
      u8 step8 = step / 8;

      u8 layer = (event_mode == SEQ_EVENT_MODE_Drum) ? 0 : ui_selected_trg_layer;
      u16 steps = (SEQ_TRG_Get8(visible_track, step8+1, layer, ui_selected_instrument) << 8) |
	          (SEQ_TRG_Get8(visible_track, step8+0, layer, ui_selected_instrument) << 0);

      if( played_step >= step && played_step < (step+16) )
	steps ^= (1 << (played_step % 16));

      int j;
      for(j=0; j<5; ++j) {
	SEQ_LCD_PrintChar(steps & 0x7);
	steps >>= 3;
      }
    }
  } else if( steps_per_item > 4 ) {
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsMedium);

    for(i=0; i<16; ++i) {
		switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 1);
			break;
			case 4:
				SEQ_LCD_CursorSet(0, 3);
			break;
			case 8:
				SEQ_LCD_CursorSet(0, 5);
			break;
			case 12:
				SEQ_LCD_CursorSet(0, 7);
			break;
		}
	
      u16 step = i*steps_per_item;
      u8 layer = (event_mode == SEQ_EVENT_MODE_Drum) ? 0 : ui_selected_trg_layer;
      u8 steps = SEQ_TRG_Get8(visible_track, step / 8, layer, ui_selected_instrument);

      if( played_step >= step && played_step < (step+8) )
	steps ^= (1 << (played_step % 8));

      int j;
      for(j=0; j<5; ++j) {
	SEQ_LCD_PrintChar(steps & 0x3);
	steps >>= 2;
      }
    }
  } else {
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsBig);

    for(i=0; i<16; ++i) {
		switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 1);
			break;
			case 4:
				SEQ_LCD_CursorSet(0, 3);
			break;
			case 8:
				SEQ_LCD_CursorSet(0, 5);
			break;
			case 12:
				SEQ_LCD_CursorSet(0, 7);
			break;
		}
	
      u8 gate_trg_asg, second_trg_asg;

      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	gate_trg_asg = 1;
	second_trg_asg = 2;
      } else {
	gate_trg_asg = SEQ_TRG_AssignmentGet(visible_track, 0);
	u8 accent_trg_asg = SEQ_TRG_AssignmentGet(visible_track, 1);
	second_trg_asg = (ui_selected_trg_layer == (gate_trg_asg-1)) ? accent_trg_asg : SEQ_TRG_AssignmentGet(visible_track, ui_selected_trg_layer);
      }

      u16 step = i*steps_per_item;
      u16 steps = gate_trg_asg ? SEQ_TRG_Get8(visible_track, step / 8, gate_trg_asg-1, ui_selected_instrument) : 0xff;

      if( second_trg_asg )
	steps |= SEQ_TRG_Get8(visible_track, step / 8, second_trg_asg-1, ui_selected_instrument) << 8;

      if( i & 1 )
	steps >>= 4;

      int j;
      for(j=0; j<4; ++j) {
	u8 accent_gate = ((steps & 0x100) ? 2 : 0) | (steps & 0x1);
	if( (step + j) == played_step ) {
	  SEQ_LCD_PrintChar(accent_gate ^ 3);
	} else {
	  SEQ_LCD_PrintChar(accent_gate);
	}
	steps >>= 1;
      }
      SEQ_LCD_PrintChar(' ');
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_STEPSEL_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
