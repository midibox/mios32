// $Id: seq_ui_trklen.c 1156 2011-03-27 18:45:18Z tk $
/*
 * Track length page
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
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4
#define ITEM_GXTY          0
#define ITEM_LENGTH        1
#define ITEM_LOOP          2
#define ITEM_QUICKSEL_MODE 3


#define QUICKSEL_MODE_LENGTH 0
#define QUICKSEL_MODE_LOOP   1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// these values are stored in local config file MBSEQ_C.V4 and can be edited there
u8 ui_quicksel_length[UI_QUICKSEL_NUM_PRESETS]      = { 3, 7, 15, 23, 31, 63, 127, 255 };
u8 ui_quicksel_loop_loop[UI_QUICKSEL_NUM_PRESETS]   = {  0,  4,  8, 12,  0,  0, 16, 24 };
u8 ui_quicksel_loop_length[UI_QUICKSEL_NUM_PRESETS] = {  3,  7, 11, 15, 15, 31, 31, 31 };


static u8 quicksel_mode = QUICKSEL_MODE_LENGTH;


/////////////////////////////////////////////////////////////////////////////
// Search for item in quick selection list
/////////////////////////////////////////////////////////////////////////////
static s32 QUICKSEL_SearchItem(u8 track)
{
  u8 search_pattern_length = SEQ_CC_Get(track, SEQ_CC_LENGTH);
  u8 search_pattern_loop = SEQ_CC_Get(track, SEQ_CC_LOOP);
  int i;

  if( quicksel_mode == QUICKSEL_MODE_LENGTH ) {
    for(i=0; i<8; ++i)
      if( ui_quicksel_length[i] == search_pattern_length )
	return i;
  } else {
    for(i=0; i<8; ++i)
      if( ui_quicksel_loop_length[i] == search_pattern_length && ui_quicksel_loop_loop[i] == search_pattern_loop )
	return i;
  }

  return -1; // item not found
}


/////////////////////////////////////////////////////////////////////////////
// Print Length or Loop value
/////////////////////////////////////////////////////////////////////////////
static s32 PrintLengthOrLoop(int value)
{
  if( value < 10 )
    SEQ_LCD_PrintFormattedString("  %d  ", value);
  else if( value < 100 )
    SEQ_LCD_PrintFormattedString("  %d ", value);
  else
    SEQ_LCD_PrintFormattedString(" %d ", value);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_LENGTH: *gp_leds = 0x0006; break;
    case ITEM_LOOP: *gp_leds = 0x0018; break;
    case ITEM_QUICKSEL_MODE: *gp_leds = 0x00c0; break;
  }

  s32 quicksel_item = QUICKSEL_SearchItem(SEQ_UI_VisibleTrackGet());
  if( quicksel_item >= 0 )
      *gp_leds |= 1 << (quicksel_item+8);

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
  u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS, incrementer);
	  
	  //ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_LOOP;
      break;

    case SEQ_UI_ENCODER_GP6:
      return 0; // not mapped

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_QUICKSEL_MODE;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16: {
      int quicksel = encoder - 8;
      int len = (quicksel_mode == QUICKSEL_MODE_LENGTH) ? ui_quicksel_length[quicksel] : ui_quicksel_loop_length[quicksel];

      if( (len+1) > num_steps ) {
	len = num_steps-1;

	char buffer[20];
	sprintf(buffer, "for %d steps", num_steps);

	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "Track only prepared", buffer);
      } else if( seq_cc_trk[visible_track].clkdiv.SYNCH_TO_MEASURE && ((int)len > (int)seq_core_steps_per_measure) ) {
	char buffer[20];
	sprintf(buffer, "active for %d steps", (int)seq_core_steps_per_measure+1);
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Synch-to-Measure is", buffer);
      }

      SEQ_UI_CC_Set(SEQ_CC_LENGTH, len);

      if( quicksel_mode == QUICKSEL_MODE_LOOP ) {
	SEQ_UI_CC_Set(SEQ_CC_LOOP, ui_quicksel_loop_loop[quicksel]);
      }

      return 1; // value has been changed
    } break;
  }

  // for GP encoders and Datawheel
  if (encoder == SEQ_UI_ENCODER_Datawheel) {
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_LENGTH: {
      if( SEQ_UI_CC_Inc(SEQ_CC_LENGTH, 0, num_steps-1, incrementer) >= 1 ) {
	if( seq_cc_trk[visible_track].clkdiv.SYNCH_TO_MEASURE && 
	    (int)SEQ_CC_Get(visible_track, SEQ_CC_LENGTH) > (int)seq_core_steps_per_measure ) {
	  char buffer[20];
	  sprintf(buffer, "active for %d steps", (int)seq_core_steps_per_measure+1);
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Synch-to-Measure is", buffer);
	}
	return 1;
      }
      return 0;
    }

    case ITEM_LOOP:
      return SEQ_UI_CC_Inc(SEQ_CC_LOOP, 0, num_steps-1, incrementer);

    case ITEM_QUICKSEL_MODE:
      return SEQ_UI_Var8_Inc(&quicksel_mode, 0, 1, incrementer);
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
      ui_selected_item = ITEM_LENGTH;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_LOOP;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP6:
      return 0; // not mapped

    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
      // toggle quicksel mode
      return Encoder_Handler((int)button, (quicksel_mode == QUICKSEL_MODE_LENGTH) ? 1 : -1);

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      return Encoder_Handler((int)button, 1);

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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk.   Length  Loop             QuickSel        Quick Selection: Length         
  // G1T1   256/256   1               Length    4    8   16   24   32   64  128  256

  // Trk.   Length  Loop             QuickSel   1    5    9   13    1    1   17   25 
  // G1T1   256/256   1               Loops     4    8   12   16   16   32   32   32 

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  s32 quicksel_item = QUICKSEL_SearchItem(SEQ_UI_VisibleTrackGet());

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.   Length  Loop  ");
	
	SEQ_LCD_CursorSet(0, 2);
	SEQ_LCD_PrintString("QuickSel: ");
 
	SEQ_LCD_CursorSet(0, 3);
	SEQ_LCD_PrintSpaces(21);
	SEQ_LCD_CursorSet(0, 4);	
 
  if( quicksel_mode == QUICKSEL_MODE_LENGTH ) {
    //SEQ_LCD_PrintString("Quick Selection: Length         ");
	SEQ_LCD_PrintSpaces(21);
  } else {
    int i;
    for(i=0; i<8; ++i) {
 		switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 4);
				break;
			case 4:
				SEQ_LCD_CursorSet(0, 6);
				break;
		}
	if( quicksel_item == i && ui_cursor_flash ) {
	SEQ_LCD_PrintSpaces(5);
      } else {
	int loop = (int)ui_quicksel_loop_loop[i] + 1;
	PrintLengthOrLoop(loop);
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(7);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(3);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(8);
  if( ui_selected_item == ITEM_LENGTH && ui_cursor_flash ) {
  } else {
    SEQ_LCD_CursorSet(7, 1);
    u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
    u16 len = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1;

    if( len > num_steps )
      SEQ_LCD_PrintFormattedString("!!!/%d", num_steps);
    else
      SEQ_LCD_PrintFormattedString("%d/%d", len, num_steps);
  }
  // to allow variable string lengths...
  SEQ_LCD_CursorSet(15, 1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LOOP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
    u16 loop = SEQ_CC_Get(visible_track, SEQ_CC_LOOP)+1;

    if( loop > num_steps )
      SEQ_LCD_PrintString("!!! ");
    else
      SEQ_LCD_PrintFormattedString("%3d ", loop);
  }

  ///////////////////////////////////////////////////////////////////////////
  //SEQ_LCD_PrintSpaces(14);
	SEQ_LCD_CursorSet(11, 2);  
  // free for sale

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_QUICKSEL_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(7);
  } else {
    if( quicksel_mode == QUICKSEL_MODE_LENGTH ) {
      SEQ_LCD_PrintString("Length     ");
    } else {
      SEQ_LCD_PrintString("Loops      ");
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<8; ++i) {
   		switch( i ) {
			case 0:
				SEQ_LCD_CursorSet(0, 5);
				break;
			case 4:
				SEQ_LCD_CursorSet(0, 7);
				break;
		}
  
  
    if( quicksel_item == i && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      int length = (int)((quicksel_mode == QUICKSEL_MODE_LENGTH) ? ui_quicksel_length[i] : ui_quicksel_loop_length[i]) + 1;

      if( length > SEQ_TRG_NumStepsGet(visible_track) )
	SEQ_LCD_PrintString(" --- ");
      else {
	PrintLengthOrLoop(length);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKLEN_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
