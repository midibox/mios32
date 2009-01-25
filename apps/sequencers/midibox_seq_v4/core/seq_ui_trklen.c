// $Id$
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


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       11
#define ITEM_GXTY          0
#define ITEM_LENGTH        1
#define ITEM_LOOP          2
#define ITEM_LENGTH_2      3
#define ITEM_LENGTH_4      4
#define ITEM_LENGTH_6      5
#define ITEM_LENGTH_8      6
#define ITEM_LENGTH_12     7
#define ITEM_LENGTH_16     8
#define ITEM_LENGTH_24     9
#define ITEM_LENGTH_32     10


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

const char quicksel_str[8][6] = { "  4  ", "  8  ", "  16 ", "  24 ", "  32 ", "  64 ", " 128 ", " 256 " };
const u8 quicksel_length[8]   = {    3,       7,       15,      23,      31 ,     63,     127,     255   };


/////////////////////////////////////////////////////////////////////////////
// Search for item in quick selection list
/////////////////////////////////////////////////////////////////////////////
static s32 QUICKSEL_SearchItem(u8 track)
{
  u8 search_pattern = SEQ_CC_Get(track, SEQ_CC_LENGTH);
  int i;
  for(i=0; i<8; ++i)
    if( quicksel_length[i] == search_pattern )
      return i;

  return -1; // item not found
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
  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
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
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      return 0; // not mapped

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      {
	int quicksel = encoder - 8;
	ui_selected_item = ITEM_LENGTH_2 + quicksel;
	SEQ_UI_CC_Set(SEQ_CC_LENGTH, quicksel_length[quicksel]);
	return 1; // value has been changed
      }
  }

  // for GP encoders and Datawheel
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u16 num_steps = SEQ_PAR_NumStepsGet(visible_track);
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_LENGTH:        return SEQ_UI_CC_Inc(SEQ_CC_LENGTH, 0, num_steps-1, incrementer);
    case ITEM_LOOP:          return SEQ_UI_CC_Inc(SEQ_CC_LOOP, 0, num_steps-1, incrementer);
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
    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
      return 0; // not mapped

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
  // Trk.   Length  Loop                             Quick Selection: Length         
  // G1T1   256/256   1                         4    8   16   24   32   64  128  256

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.   Length  Loop                     ");
  SEQ_LCD_PrintString("        Quick Selection: Length         ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(7);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(3);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(8);
  } else {
    u16 num_steps = SEQ_PAR_NumStepsGet(visible_track);
    u16 len = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH)+1;
    // TODO: a "center string" function would be useful here!
    if( num_steps >= 100 )
      if( len >= 100 )
	SEQ_LCD_PrintFormattedString("%3d/%3d", len, num_steps);
      else
	SEQ_LCD_PrintFormattedString("%2d/%3d ", len, num_steps);
    else
      SEQ_LCD_PrintFormattedString(" %2d/%2d ", len, num_steps);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_LOOP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", SEQ_CC_Get(visible_track, SEQ_CC_LOOP)+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(21);
  // free for sale

  ///////////////////////////////////////////////////////////////////////////
  s32 quicksel_item = QUICKSEL_SearchItem(SEQ_UI_VisibleTrackGet());

  int i;
  for(i=0; i<8; ++i) {
    if( quicksel_item == i && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintString((char *)quicksel_str[i]);
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
