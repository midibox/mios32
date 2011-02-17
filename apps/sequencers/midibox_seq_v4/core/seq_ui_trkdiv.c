// $Id$
/*
 * Track clock divider page
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

#define NUM_OF_ITEMS       12
#define ITEM_GXTY          0
#define ITEM_DIVIDER       1
#define ITEM_TRIPLET       2
#define ITEM_SYNCH_TO_MEASURE 3
#define ITEM_TIMEBASE_1    4
#define ITEM_TIMEBASE_2    5
#define ITEM_TIMEBASE_4    6
#define ITEM_TIMEBASE_8    7
#define ITEM_TIMEBASE_8T   8
#define ITEM_TIMEBASE_16   9
#define ITEM_TIMEBASE_16T  10
#define ITEM_TIMEBASE_32   11


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char quicksel_str[8][6] =  { "  1  ", "  2  ", "  4  ", "  8  ", "  8T ", "  16 ", "  16T", "  32 " };
static const u16 quicksel_timebase[8] = {  255,      127,     63,      31,   31|0x100,    15,   15|0x100,    7   };


/////////////////////////////////////////////////////////////////////////////
// Search for item in quick selection list
/////////////////////////////////////////////////////////////////////////////
static s32 QUICKSEL_SearchItem(u8 track)
{
  u16 search_pattern = SEQ_CC_Get(track, SEQ_CC_CLK_DIVIDER) | 
                    ((SEQ_CC_Get(track, SEQ_CC_CLKDIV_FLAGS)&2)<<7);
  int i;
  for(i=0; i<8; ++i)
    if( quicksel_timebase[i] == search_pattern )
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
    case ITEM_DIVIDER: *gp_leds = 0x0002; break;
    case ITEM_TRIPLET: *gp_leds = 0x000c; break;
    case ITEM_SYNCH_TO_MEASURE: *gp_leds = 0x0070; break;
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
      ui_selected_item = ITEM_DIVIDER;
      break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_TRIPLET;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_SYNCH_TO_MEASURE;
      break;

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
	ui_selected_item = ITEM_TIMEBASE_1 + quicksel;
	SEQ_UI_CC_Set(SEQ_CC_CLK_DIVIDER, quicksel_timebase[quicksel] & 0xff);
	SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<1), (quicksel_timebase[quicksel] & 0x100) ? (1<<1) : 0);
	return 1; // value has been changed
      }
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_DIVIDER:       return SEQ_UI_CC_Inc(SEQ_CC_CLK_DIVIDER, 0, 255, incrementer);
    case ITEM_TRIPLET:       return SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<1), (incrementer >= 0) ? (1<<1) : 0);
    case ITEM_SYNCH_TO_MEASURE: return SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<0), (incrementer >= 0) ? (1<<0) : 0);
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

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP2:
      ui_selected_item = ITEM_DIVIDER;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
      return Encoder_Handler((int)button, (SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<1)) ? -1 : 1); // toggle flag

    case SEQ_UI_BUTTON_GP5:
    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
      return Encoder_Handler((int)button, (SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<0)) ? -1 : 1); // toggle flag

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
  // Trk.  Clock Divider  Synch to Measure          Quick Selection: Timebase        
  // G1T1    4 (normal)         yes            1    2    4    8    8T   16  16T   32

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.  Clock Divider  Synch to Measure   ");
  SEQ_LCD_PrintString("       Quick Selection: Timebase        ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(2);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_DIVIDER && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", SEQ_CC_Get(visible_track, SEQ_CC_CLK_DIVIDER)+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_TRIPLET && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(11);
  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<1)) ? "(triplet)  " : "(normal)   ");
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_SYNCH_TO_MEASURE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(19);
  } else {
    SEQ_LCD_PrintSpaces(6);
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<0)) ? "yes" : "no ");
    SEQ_LCD_PrintSpaces(10);
  }

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
s32 SEQ_UI_TRKDIV_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
