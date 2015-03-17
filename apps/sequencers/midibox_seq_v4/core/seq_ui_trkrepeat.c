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

#define NUM_OF_ITEMS         6
#define ITEM_GXTY            0
#define ITEM_TRG_SELECT      1
#define ITEM_REPEAT_ENABLE   2
#define ITEM_RECORD          3
#define ITEM_REPEAT_PATTERN  4
#define ITEM_REPEAT_LENGTH   5


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  if( seq_ui_button_state.SELECT_PRESSED ) {
    seq_live_repeat_t *slot = SEQ_LIVE_CurrentSlotGet();
    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
    *gp_leds = pattern->gate;
  } else {
    switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_TRG_SELECT: *gp_leds = 0x0002; break;
    case ITEM_REPEAT_ENABLE: *gp_leds = 0x0004; break;
    case ITEM_RECORD: *gp_leds = 0x0040; break;
    case ITEM_REPEAT_PATTERN: *gp_leds = 0x0f00; break;
    case ITEM_REPEAT_LENGTH: *gp_leds = 0x1000; break;
    }
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

  if( seq_ui_button_state.SELECT_PRESSED && encoder < 16 ) {
    if( slot->pattern >= SEQ_LIVE_NUM_ARP_PATTERNS )
      return -1; // invalid pattern

    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
    u16 mask = (1 << encoder);
    u8 gate = (pattern->gate & mask) ? 1 : 0;
    u8 accent = (pattern->accent & mask) ? 3 : 0; // and not 3: we want to toggle between 0, 1 and 2; accent set means that gate is set as well
    u8 value = gate | accent;
    if( value >= 2 ) value = 2;

    if( incrementer == 0 ) {
      // button toggles between values
      if( ++value >= 3 )
	value = 0;
    } else {
      // encoder moves between 3 states
      if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) <= 0 ) {
	return -1;
      }
    }

    if( value == 0 ) {
      pattern->gate &= ~mask;
      pattern->accent &= ~mask;
    } else if( value == 1 ) {
      pattern->gate |= mask;
      pattern->accent &= ~mask;
    } else if( value == 2 ) {
      pattern->gate |= mask;
      pattern->accent |= mask;
    }

    return 1;
  }

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
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      return -1; // not used

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_RECORD;
      break;

    case SEQ_UI_ENCODER_GP8:
      SEQ_UI_PageSet(SEQ_UI_PAGE_TRKLIVE);
      return 0;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_REPEAT_PATTERN;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_REPEAT_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP14:
      return -1; // not used

    case SEQ_UI_ENCODER_GP15:
      SEQ_UI_UTIL_CopyLivePattern();
      SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Pattern copied", "into Edit Buffer");
      return 1;

    case SEQ_UI_ENCODER_GP16:
      if( SEQ_UI_UTIL_PasteLivePattern() < 0 ) {
	SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Edit Buffer", "is empty!");
      } else {
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 2000, "Pattern copied", "from Edit Buffer");
      }
      return 1;
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

  case ITEM_RECORD: {
    if( incrementer == 0 )
      seq_record_state.ENABLED = seq_record_state.ENABLED ? 0 : 1;
    else
      seq_record_state.ENABLED = (incrementer > 0);
    return 1;
  } break;

  case ITEM_REPEAT_PATTERN: {
    u8 value = slot->pattern;
    if( SEQ_UI_Var8_Inc(&value, 0, SEQ_LIVE_NUM_ARP_PATTERNS-1, incrementer) > 0 ) {
      slot->pattern = value;
      return 1;
    }
    return 0;
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
  // Trk. Drum Repeat              Rec.  Live       Pattern      Length              
  // G1T1  BD    on                off   Page 1 *.*.*.*.*.*.*.*.   75%     Copy Paste

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
  SEQ_LCD_PrintString("Repeat              Rec.  Live       Pattern      Length ");
  if( seq_ui_button_state.SELECT_PRESSED ) {
    SEQ_LCD_PrintString("    Edit Mode");
  } else {
    SEQ_LCD_PrintSpaces(13);
  }

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
  SEQ_LCD_PrintSpaces(15);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RECORD && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_record_state.ENABLED ? " on" : "off");
  }
  SEQ_LCD_PrintSpaces(3);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("Page");

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REPEAT_PATTERN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%2d ", slot->pattern + 1);
  }

  ///////////////////////////////////////////////////////////////////////////
  {
    seq_live_arp_pattern_t *pattern = (seq_live_arp_pattern_t *)&seq_live_arp_pattern[slot->pattern];
    int step;
    u16 mask = 0x0001;
    for(step=0; step<16; ++step, mask <<= 1) {
      u8 gate = (pattern->gate & mask) ? 1 : 0;
      u8 accent = (pattern->accent & mask) ? 2 : 0;
      SEQ_LCD_PrintChar(gate | accent);
    }
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_REPEAT_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGatelength(slot->len + 1);
  }

  ///////////////////////////////////////////////////////////////////////////

  SEQ_LCD_PrintSpaces(5);

  if( seq_ui_button_state.SELECT_PRESSED ) {
    SEQ_LCD_PrintSpaces(10);
  } else {
    SEQ_LCD_PrintString("Copy Paste");
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

  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_DrumSymbolsBig);

  return 0; // no error
}
