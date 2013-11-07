// $Id: seq_ui_trkmorph.c 1806 2013-06-16 19:17:37Z tk $
/*
 * Track Morphing page
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
#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_morph.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           4
#define ITEM_GXTY              0
#define ITEM_MORPH_MODE        1
#define ITEM_MORPH_DST         2
#define ITEM_MORPH_VALUE       3


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  *gp_leds = ((2 << (SEQ_MORPH_ValueGet() / 16))-1) << 8;

  if( ui_cursor_flash ) { // if flashing flag active: no LED flag set
    if( ui_selected_item == ITEM_MORPH_VALUE ) // flashes only if selected, otherwise permanently active
      *gp_leds = 0;
    return 0;
  }

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds |= 0x0001; break;
    case ITEM_MORPH_MODE: *gp_leds |= 0x0002; break;
    case ITEM_MORPH_DST: *gp_leds |= 0x000c; break;
    case ITEM_MORPH_VALUE: *gp_leds |= 0x0080; break;
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
  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_MORPH_MODE;
      break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_MORPH_DST;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
      return -1; // not used (yet)

    case SEQ_UI_ENCODER_GP8:
    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_MORPH_VALUE;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_MORPH_MODE:    return SEQ_UI_CC_Inc(SEQ_CC_MORPH_MODE, 0, 1, incrementer);
    case ITEM_MORPH_DST:     return SEQ_UI_CC_Inc(SEQ_CC_MORPH_DST, 0, 255, incrementer);
    case ITEM_MORPH_VALUE:   {
      u8 value = SEQ_MORPH_ValueGet();
      if( SEQ_UI_Var8_Inc(&value, 0, 127, incrementer) > 0 ) {
	SEQ_MORPH_ValueSet(value);

	// send to external
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_MORPH, value, 0);

	return 1;
      }
      return 0;
    } break;
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
      return 1;

    case SEQ_UI_BUTTON_GP2:
      ui_selected_item = ITEM_MORPH_MODE;
      break;

    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
      ui_selected_item = ITEM_MORPH_DST;
      break;

    case SEQ_UI_BUTTON_GP5:
    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
      return -1; // not used (yet)

    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_MORPH_VALUE;
      break;

    case SEQ_UI_BUTTON_GP9:
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      // direct morph value selection
      SEQ_MORPH_ValueSet(((u8)button-SEQ_UI_BUTTON_GP9) * 16);
      return 1; // value always changed

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
  // Trk. Mode  Dst.Range               ValueMorphing controlled by All /Chn 1 CC#  1
  // G1T1  on    17..32                  100    <######################          >


  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Trk. Mode  Dst.Range               Value");

  if( !seq_midi_in_ext_ctrl_channel || seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH] >= 0x80 ) {
    SEQ_LCD_PrintString("Please enable Ext. Ctrl in the MIDI page");
  } else {
    SEQ_LCD_PrintFormattedString("Morphing controlled by %s/Chn%2d CC#%3d",
				 seq_midi_in_ext_ctrl_port ? SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_ext_ctrl_port)) : " All",
				 seq_midi_in_ext_ctrl_channel,
				 seq_midi_in_ext_ctrl_asg[SEQ_MIDI_IN_EXT_CTRL_MORPH]);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MORPH_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(SEQ_CC_Get(visible_track, SEQ_CC_MORPH_MODE) ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MORPH_DST && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(8);
  } else {
    int dst_begin = SEQ_CC_Get(visible_track, SEQ_CC_MORPH_DST) + 1;
    int dst_end = dst_begin + SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
    if( dst_end > 256 )
      dst_end = 256;

    SEQ_LCD_PrintFormattedString("%3d..%d  ", dst_begin, dst_end);
  }
  SEQ_LCD_CursorSet(19, 1); // set back cursor
  SEQ_LCD_PrintSpaces(17);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MORPH_VALUE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", SEQ_MORPH_ValueGet());
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString("   <");
  SEQ_LCD_PrintLongHBar(SEQ_MORPH_ValueGet());
  SEQ_LCD_PrintString(">   ");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKMORPH_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show a horizontal morph meter
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

  return 0; // no error
}
