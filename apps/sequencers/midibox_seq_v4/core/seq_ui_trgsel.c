// $Id$
/*
 * Trigger selection page (entered with Trigger Layer C button)
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

#include "seq_trg.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  if( event_mode == SEQ_EVENT_MODE_Drum )
    *gp_leds = 1 << ui_selected_instrument;
  else
    *gp_leds = 1 << ui_selected_trg_layer;

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

  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
    // select new layer/instrument

    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      if( encoder >= SEQ_TRG_NumInstrumentsGet(visible_track) )
	return -1;
      ui_selected_instrument = encoder;
    } else {
      if( encoder >= SEQ_TRG_NumLayersGet(visible_track) )
	return -1;
      ui_selected_trg_layer = encoder;
    }

#if DEFAULT_BEHAVIOUR_BUTTON_TRG_LAYER
    // if toggle function active: jump back to previous menu
    // this is especially useful for the emulated MBSEQ, where we can only click on a single button
    // (trigger gets deactivated when clicking on GP button or moving encoder)
    if( seq_ui_button_state.TRG_LAYER_SEL ) {
      seq_ui_button_state.TRG_LAYER_SEL = 0;
      SEQ_UI_PageSet(ui_trglayer_prev_page);
    }
#endif

    return 1; // value changed
  } else if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, SEQ_TRG_NumInstrumentsGet(visible_track)-1, incrementer);
    } else {
      return SEQ_UI_Var8_Inc(&ui_selected_trg_layer, 0, SEQ_TRG_NumLayersGet(visible_track)-1, incrementer);
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
  // layout normal mode:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Gate Acc. Roll Glide Skip R.G  R.V No Fx                                        
  //   A    B    C    D    E    F    G    H                                          

  // layout drum mode (lower line shows drum labels):
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Select Drum Instrument:                                                         
  //  BD   SD   LT   MT   HT   CP   MA   RS   CB   CY   OH   CH  Smp1 Smp2 Smp3 Smp4 
  // ...horizontal VU meters...

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  if( high_prio && event_mode == SEQ_EVENT_MODE_Drum ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters

    SEQ_LCD_CursorSet(0, 1);

    u8 drum;
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);
    for(drum=0; drum<num_instruments; ++drum)
      SEQ_LCD_PrintHBar((seq_layer_vu_meter[drum] >> 3) & 0xf);

    return 0; // no error
  }

  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    u8 num_instruments = SEQ_TRG_NumInstrumentsGet(visible_track);

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 0);

    int i;
    for(i=0; i<num_instruments; ++i)
      if( i == ui_selected_instrument && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(5);
      else
	SEQ_LCD_PrintTrackDrum(visible_track, i, (char *)seq_core_trk[visible_track].name);

    SEQ_LCD_PrintSpaces(80 - (5*num_instruments));
  } else {
    u8 num_layers = SEQ_TRG_NumLayersGet(visible_track);

    SEQ_LCD_CursorSet(0, 0);
    int i;
    for(i=0; i<num_layers; ++i) {
      if( i == ui_selected_trg_layer && ui_cursor_flash )
	SEQ_LCD_PrintSpaces(5);
      else
	SEQ_LCD_PrintString(SEQ_TRG_AssignedTypeStr(visible_track, i));
    }

    SEQ_LCD_PrintSpaces(80 - (5*num_layers));

    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 1);

    for(i=0; i<num_layers; ++i) {
      SEQ_LCD_PrintChar(' ');
      SEQ_LCD_PrintChar((i == ui_selected_trg_layer) ? '>' : ' ');

      if( i == ui_selected_trg_layer && ui_cursor_flash )
	SEQ_LCD_PrintChar(' ');
      else {
	SEQ_LCD_PrintChar('A' + i);
      }

      SEQ_LCD_PrintChar((i == ui_selected_trg_layer) ? '<' : ' ');
      SEQ_LCD_PrintChar(' ');
    }

    SEQ_LCD_PrintSpaces(80 - (5*num_layers));
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRGSEL_Init(u32 mode)
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
