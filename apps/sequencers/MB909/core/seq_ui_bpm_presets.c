// $Id: seq_ui_bpm_presets.c 690 2009-08-04 22:49:33Z tk $
/*
 * BPM presets page
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

#include "seq_ui.h"
#include "seq_lcd.h"
#include "seq_bpm.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 1 << seq_core_bpm_preset_num;

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
  return -1; // don't use any encoder function
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

  // GP button selects preset
  if( button <= SEQ_UI_BUTTON_GP16 ) {
    // set preset
    seq_core_bpm_preset_num = button;
    // change Tempo
    SEQ_CORE_BPM_Update(seq_core_bpm_preset_tempo[seq_core_bpm_preset_num], seq_core_bpm_preset_ramp[seq_core_bpm_preset_num]);
#if 0
    // and enter BPM page again
    SEQ_UI_PageSet(SEQ_UI_PAGE_BPM);
#endif
    // (NO!)
    return 1;
  }

  // TODO
  // too bad, that the menu handling concept doesn't allow to react on exit button here
  // this would allow to exit preset selection mode w/o exiting BPM page

  return -1; // ignore remaining buttons
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  if( high_prio )
    return 0; // there are no high-priority updates

  int i;

  MIOS32_LCD_CursorSet(0, 0);
  for(i=0; i<16; ++i) {
    float bpm = seq_core_bpm_preset_tempo[i];
	
		switch( i ) {
		case 4:
			MIOS32_LCD_CursorSet(0, 2);
		break;
		case 8:
			MIOS32_LCD_CursorSet(0, 4);
		break;
		case 12:
			MIOS32_LCD_CursorSet(0, 6);
		break;
	}
	
    if( i == seq_core_bpm_preset_num ) {
      MIOS32_LCD_PrintFormattedString(ui_cursor_flash ? ">   <" : ">%3d<", (int)bpm);
    } else {
      MIOS32_LCD_PrintFormattedString(" %3d ", (int)bpm);
    }
  }

  MIOS32_LCD_CursorSet(0, 1);
  for(i=0; i<16; ++i) {
    float ramp = seq_core_bpm_preset_ramp[i];
	
	switch( i ) {
		case 4:
			MIOS32_LCD_CursorSet(0, 3);
		break;
		case 8:
			MIOS32_LCD_CursorSet(0, 5);
		break;
		case 12:
			MIOS32_LCD_CursorSet(0, 7);
		break;
	}
	
    if( i == seq_core_bpm_preset_num ) {
      MIOS32_LCD_PrintFormattedString(ui_cursor_flash ? ">   <" : ">%2ds<", (int)ramp);
    } else {
      MIOS32_LCD_PrintFormattedString(" %2ds ", (int)ramp);
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BPM_PRESETS_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
