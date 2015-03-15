// $Id: seq_ui_bpm_presets.c 690 2009-08-04 22:49:33Z tk $
/*
 * Live Repeat page
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
#include "seq_live.h"
#include "seq_record.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS         3
#define ITEM_GXTY            0
#define ITEM_TRG_SELECT      1
#define ITEM_REPEAT_ENABLE   2
#define ITEM_REPEAT_LENGTH   3


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static const char quicksel_str[8][6] = { "  1  ", "  2  ", "  4  ", "  6  ", "  8  ", "  16 ", "  24 ", "  32 " };
static const u16 quicksel_repeat[8]  = {  256,      128,     64,      48,      32,       16,      12,      8    };

/////////////////////////////////////////////////////////////////////////////
// Search for item in quick selection list
/////////////////////////////////////////////////////////////////////////////
static s32 QUICKSEL_SearchItem(u8 track)
{
  u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
  u8 ix = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;
  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[ix];    
  u16 search_pattern = slot->repeat_ticks;

  int i;
  for(i=0; i<8; ++i)
    if( quicksel_repeat[i] == search_pattern )
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
    case ITEM_TRG_SELECT: *gp_leds = 0x0002; break;
    case ITEM_REPEAT_ENABLE: *gp_leds = 0x0004; break;
    case ITEM_REPEAT_LENGTH: *gp_leds = 0x0010; break;
  }

  {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    s32 quicksel_item = QUICKSEL_SearchItem(visible_track);
    if( quicksel_item >= 0 )
      *gp_leds |= 1 << (quicksel_item+8);
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
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 ix = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;
  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[ix];    

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_TRG_SELECT;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_REPEAT_ENABLE;
      break;

    case SEQ_UI_ENCODER_GP4:
      return -1; // not used

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_REPEAT_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
      return -1; // not used

    case SEQ_UI_ENCODER_GP8:
      SEQ_UI_PageSet(SEQ_UI_PAGE_TRKLIVE);
      return 0;
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
      slot->repeat_ticks = quicksel_repeat[quicksel];

      return 1; // value has been changed
    }
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
  case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);

  case ITEM_TRG_SELECT: {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      u8 num_drums = SEQ_TRG_NumInstrumentsGet(visible_track);
      return SEQ_UI_Var8_Inc(&ui_selected_instrument, 0, num_drums-1, incrementer);
    } else {
      //u8 num_t_layers = SEQ_TRG_NumLayersGet(visible_track);
      //return SEQ_UI_Var8_Inc(&ui_selected_trg_layer, 0, num_t_layers-1, incrementer);
      return -1;
    }
  } break;

  case ITEM_REPEAT_ENABLE: {
    if( incrementer == 0 )
      slot->enabled = slot->enabled ? 0 : 1;
    else
      slot->enabled = (incrementer > 0);
    return 1;
  } break;

  case ITEM_REPEAT_LENGTH: {
    u8 value = slot->len;
    if( SEQ_UI_Var8_Inc(&value, 0, 94, incrementer) > 0 ) {
      slot->len = value;
      return 1;
    }
    return 0;
  } break;
  }

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

  if( button <= SEQ_UI_BUTTON_GP16 ) {
    // -> forward to encoder handler
    return Encoder_Handler((int)button, 0);
  }

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

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Drum Repeat    Length          Live         Quick Selection: Repeat        
  // G1T1  BD    on        75%           Page  1    2    4    6    8    16   24   32 

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  u8 ix = (event_mode == SEQ_EVENT_MODE_Drum) ? ui_selected_instrument : 0;
  seq_live_repeat_t *slot = (seq_live_repeat_t *)&seq_live_repeat[ix];    


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  if( event_mode == SEQ_EVENT_MODE_Drum ) {
    SEQ_LCD_PrintString("Trk. Drum ");
  } else {
    SEQ_LCD_PrintString("Trk.      ");
  }
  SEQ_LCD_PrintString("Repeat    Length          Live         Quick Selection: Repeat        ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_TRG_SELECT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    if( event_mode == SEQ_EVENT_MODE_Drum ) {
      SEQ_LCD_PrintTrackDrum(visible_track, ui_selected_instrument, (char *)seq_core_trk[visible_track].name);
    } else {
      SEQ_LCD_PrintSpaces(5);
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REPEAT_ENABLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintString(slot->enabled ? "  on " : "  off");
  }
  SEQ_LCD_PrintSpaces(6);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REPEAT_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGatelength(slot->len + 1);
  }

  SEQ_LCD_PrintSpaces(11);
  SEQ_LCD_PrintString("Page");

  ///////////////////////////////////////////////////////////////////////////
  s32 quicksel_item = QUICKSEL_SearchItem(visible_track);

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
s32 SEQ_UI_TRKREPEAT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
