// $Id$
/*
 * Menu page (visible when EXIT button is pressed)
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
#include <seq_midi_out.h>
#include <seq_bpm.h>
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_file_bm.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 last_bookmark = 0;
static u8 store_state;

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static void Button_StoreRequest(u32 state);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  *gp_leds = (1 << last_bookmark);

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
    last_bookmark = encoder;
    SEQ_UI_Bookmark_Restore(last_bookmark);
    return 1;
  }

  // all other encoders
  return -1;
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16) ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    if( depressed ) {
      if( store_state == 1 ) { // depressed within 1 second: select bookmark
	SEQ_UI_Bookmark_Restore(last_bookmark);
      } else {
	SEQ_UI_UnInstallDelayedActionCallback(Button_StoreRequest);
      }
      return 0;
    }
    last_bookmark = button;
    store_state = 1;
    SEQ_UI_InstallDelayedActionCallback(Button_StoreRequest, 500, 0);

    return 1;
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
    case SEQ_UI_BUTTON_Up:
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Left:
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //    Press GP Button to recall Bookmark orhold GP Button to store new Bookmark    
  // BM 1 BM 2 BM 3 BM 4 BM 5 BM 6 BM 7 BM 8 BM 9 BM10 BM11 BM12 BM13 BM14 BM15 BM16 

  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("   Press GP Button to recall Bookmark orhold GP Button to store new Bookmark    ");

  SEQ_LCD_CursorSet(0, 1);
  int i, j;
  for(i=0; i<SEQ_UI_BOOKMARKS_NUM; ++i) {
    if( i >= 16 )
      break; // just to ensure...
    seq_ui_bookmark_t *bm = (seq_ui_bookmark_t *)&seq_ui_bookmarks[i];

    if( ui_cursor_flash && (i == last_bookmark) )
      SEQ_LCD_PrintSpaces(5);
    else {
      for(j=0; j<5; ++j)
	if( bm->name[j] == 0 )
	  break;
	else
	  SEQ_LCD_PrintChar(bm->name[j]);

      while( j++ < 5 )
	SEQ_LCD_PrintChar(' ');
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BOOKMARKS_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  store_state = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// help function for delayed store action
/////////////////////////////////////////////////////////////////////////////
static void Button_StoreRequest(u32 state)
{
  if( store_state == 1 ) {
    SEQ_UI_Msg(last_bookmark >= 8 ? SEQ_UI_MSG_DELAYED_ACTION_R : SEQ_UI_MSG_DELAYED_ACTION, 2001,
	       "Hold Button", "to store Bookmark!");
    SEQ_UI_InstallDelayedActionCallback(Button_StoreRequest, 2000, 0);
    store_state = 2;
  } else {
    store_state = 0;
    // store into selected slot
    SEQ_UI_Bookmark_Store(last_bookmark);

    SEQ_UI_Msg(last_bookmark >= 8 ? SEQ_UI_MSG_USER_R : SEQ_UI_MSG_USER, 2000,
	       "Storing", "Bookmark!");      
#if !defined(MIOS32_FAMILY_EMULATION)
    // this yield ensures, that the display will be updated before storing the file
    taskYIELD();
#endif
    // and store file
    MUTEX_SDCARD_TAKE;
    s32 error = SEQ_FILE_BM_Write();
    MUTEX_SDCARD_GIVE;
    if( error < 0 )
      SEQ_UI_SDCardErrMsg(2000, error);
    else {
      // return to bookmarked page
      SEQ_UI_PageSet((seq_ui_page_t)seq_ui_bookmarks[last_bookmark].page);

      // and print message
      SEQ_UI_Msg(last_bookmark >= 8 ? SEQ_UI_MSG_USER_R : SEQ_UI_MSG_USER, 2000,
		 "Bookmark", "stored!");      
    }
  }
}
