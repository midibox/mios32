// $Id$
/*
 * Trigger selection page (entered with Parameter Layer C button)
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
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_hwcfg.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 1 << ui_selected_par_layer;

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

  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    // select new layer

    if( encoder >= SEQ_PAR_NumLayersGet(visible_track) )
      return -1;
    ui_selected_par_layer = encoder;

    if( seq_hwcfg_button_beh.par_layer ) {
      // if toggle function active: jump back to previous menu
      // this is especially useful for the emulated MBSEQ, where we can only click on a single button
      // (trigger gets deactivated when clicking on GP button or moving encoder)
      seq_ui_button_state.PAR_LAYER_SEL = 0;
      SEQ_UI_PageSet(ui_parlayer_prev_page);
    }

    return 1; // value changed
  } else if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    return SEQ_UI_Var8_Inc(&ui_selected_par_layer, 0, SEQ_PAR_NumLayersGet(visible_track)-1, incrementer);
  }

  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported gp_button
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
      return -1; // unsupported (yet)

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
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 num_layers = SEQ_PAR_NumLayersGet(visible_track);

  if( high_prio && event_mode != SEQ_EVENT_MODE_Drum ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters

    SEQ_LCD_CursorSet(0, 1);

    u8 layer;
    for(layer=0; layer<num_layers; ++layer)
      SEQ_LCD_PrintHBar((seq_layer_vu_meter[layer] >> 3) & 0xf);

    return 0; // no error
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  int i;
  for(i=0; i<num_layers; ++i)
    if( i == ui_selected_par_layer && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(5);
    else
      SEQ_LCD_PrintString((char *)SEQ_PAR_AssignedTypeStr(visible_track, i));

  SEQ_LCD_PrintSpaces(80 - (5*num_layers));

  ///////////////////////////////////////////////////////////////////////////
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_CursorSet(0, 1);

    for(i=0; i<num_layers; ++i) {
      SEQ_LCD_PrintChar(' ');
      SEQ_LCD_PrintChar((i == ui_selected_par_layer) ? '>' : ' ');

      if( i == ui_selected_par_layer && ui_cursor_flash )
	SEQ_LCD_PrintChar(' ');
      else {
	SEQ_LCD_PrintChar('A' + i);
      }

      SEQ_LCD_PrintChar((i == ui_selected_par_layer) ? '<' : ' ');
      SEQ_LCD_PrintChar(' ');
    }
    SEQ_LCD_PrintSpaces(80 - (5*num_layers));
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PARSEL_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show horizontal VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

  return 0; // no error
}
