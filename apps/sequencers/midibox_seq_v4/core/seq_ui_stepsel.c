// $Id$
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

#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = 1 << ui_selected_step_view;

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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    // select new step view
    ui_selected_step_view = encoder;

    // select step within view
    ui_selected_step = (ui_selected_step_view << 4) | (ui_selected_step & 0xf);

#if DEFAULT_BEHAVIOUR_BUTTON_STEPVIEW
    // if toggle function active: jump back to previous menu
    // this is especially useful for the emulated MBSEQ, where we can only click on a single button
    // (stepview gets deactivated when clicking on GP button)
    if( seq_ui_button_state.STEPVIEW ) {
      seq_ui_button_state.STEPVIEW = 0;
      SEQ_UI_PageSet(ui_stepview_prev_page);
    }
#endif

    return 1; // value changed
  } else if( encoder == SEQ_UI_ENCODER_Datawheel ) {
    return SEQ_UI_Var8_Inc(&ui_selected_step_view, 0, SEQ_CORE_NUM_STEPS/16, incrementer);
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
  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // >Pos< 17-  33-  49-  65-  81-  97- 113- 129- 145- 161- 177- 193- 209- 225- 241- 
  // > 15<  32   48   64   80   96  112  128  144  160  176  192  208  224  240  256

  // "Pos xxx is just a temporary marker to print the current position

  // this requires high-prio updates!
#if 0
  if( !high_prio )
    return 0; // there are no high-priority updates
#endif


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  int played_step = seq_core_trk[visible_track].step + 1;
  int played_view = (played_step-1) / 16;

  int i;

  SEQ_LCD_CursorSet(0, 0);
  for(i=0; i<16; ++i)
    if( i == ui_selected_step_view && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(5);
    else if( i == played_view )
      SEQ_LCD_PrintString(">Pos<");
    else
      SEQ_LCD_PrintFormattedString("%3d- ", i*16+1);

  SEQ_LCD_CursorSet(0, 1);
  for(i=0; i<16; ++i)
    if( i == ui_selected_step_view && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(5);
    else if( i == played_view )
      SEQ_LCD_PrintFormattedString(">%3d<", played_step);
    else
      SEQ_LCD_PrintFormattedString("  %3d", i*16+16);

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
