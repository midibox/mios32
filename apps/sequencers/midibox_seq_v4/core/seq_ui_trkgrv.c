// $Id$
/*
 * Track Groove page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include "seq_cc.h"
#include "seq_groove.h"
#include "seq_file.h"
#include "seq_file_g.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           8
#define ITEM_GXTY              0
#define ITEM_GROOVE_STYLE      1
#define ITEM_GROOVE_VALUE      2
#define ITEM_GROOVE_STEP       3
#define ITEM_GROOVE_DELAY      4
#define ITEM_GROOVE_LENGTH     5
#define ITEM_GROOVE_VELOCITY   6
#define ITEM_GROOVE_NUM_STEPS  7


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 edit_step;
static u8 store_file_required;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_GROOVE_STYLE: *gp_leds = 0x000e; break;
    case ITEM_GROOVE_VALUE: *gp_leds = 0x0030; break;
    case ITEM_GROOVE_STEP: *gp_leds = 0x0100; break;
    case ITEM_GROOVE_DELAY: *gp_leds = 0x0200; break;
    case ITEM_GROOVE_LENGTH: *gp_leds = 0x0400; break;
    case ITEM_GROOVE_VELOCITY: *gp_leds = 0x0800; break;
    case ITEM_GROOVE_NUM_STEPS: *gp_leds = 0x1000; break;
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

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_GROOVE_STYLE;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_GROOVE_VALUE;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_GROOVE_STEP;
      break;
      
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_GROOVE_DELAY;
      break;
      
    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_GROOVE_LENGTH;
      break;
      
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_GROOVE_VELOCITY;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_GROOVE_NUM_STEPS;
      break;

    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped yet
  }

  // for GP encoders and Datawheel
  u8 grooves_total = SEQ_GROOVE_NUM_PRESETS+SEQ_GROOVE_NUM_TEMPLATES;
  u8 selected_groove = SEQ_CC_Get(visible_track, SEQ_CC_GROOVE_STYLE);
  s32 groove_template = selected_groove - SEQ_GROOVE_NUM_PRESETS; // negative if not a custom template
  seq_groove_entry_t *g;
  if( selected_groove >= SEQ_GROOVE_NUM_PRESETS )
    g = (seq_groove_entry_t *)&seq_groove_templates[selected_groove-SEQ_GROOVE_NUM_PRESETS];
  else
    g = (seq_groove_entry_t *)&seq_groove_presets[selected_groove];

  switch( ui_selected_item ) {
    case ITEM_GXTY:              return SEQ_UI_GxTyInc(incrementer);
    case ITEM_GROOVE_STYLE:      return SEQ_UI_CC_Inc(SEQ_CC_GROOVE_STYLE, 0, grooves_total-1, incrementer);
    case ITEM_GROOVE_VALUE:      return SEQ_UI_CC_Inc(SEQ_CC_GROOVE_VALUE, 0, 127, incrementer);
    case ITEM_GROOVE_STEP:       return SEQ_UI_Var8_Inc(&edit_step, 0, g->num_steps-1, incrementer);

    case ITEM_GROOVE_DELAY: {
      if( groove_template < 0 || groove_template >= SEQ_GROOVE_NUM_TEMPLATES )
	return 0;
      u8 value = (u8)seq_groove_templates[groove_template].add_step_delay[edit_step] + 128;
      if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) > 0 ) {
	seq_groove_templates[groove_template].add_step_delay[edit_step] = (s8)(value - 128);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_GROOVE_LENGTH: {
      if( groove_template < 0 || groove_template >= SEQ_GROOVE_NUM_TEMPLATES )
	return 0;
      u8 value = (u8)seq_groove_templates[groove_template].add_step_length[edit_step] + 128;
      if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) > 0 ) {
	seq_groove_templates[groove_template].add_step_length[edit_step] = (s8)(value - 128);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_GROOVE_VELOCITY: {
      if( groove_template < 0 || groove_template >= SEQ_GROOVE_NUM_TEMPLATES )
	return 0;
      u8 value = (u8)seq_groove_templates[groove_template].add_step_velocity[edit_step] + 128;
      if( SEQ_UI_Var8_Inc(&value, 0, 255, incrementer) > 0 ) {
	seq_groove_templates[groove_template].add_step_velocity[edit_step] = (s8)(value - 128);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_GROOVE_NUM_STEPS: {
      if( groove_template < 0 || groove_template >= SEQ_GROOVE_NUM_TEMPLATES )
	return 0;
      u8 value = (u8)seq_groove_templates[groove_template].num_steps;
      if( SEQ_UI_Var8_Inc(&value, 1, 16, incrementer) > 0 ) {
	seq_groove_templates[groove_template].num_steps = value;
	store_file_required = 1;
	return 1;
      }
      return 0;
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

  u8 visible_track = SEQ_UI_VisibleTrackGet();

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP15 ) {
#endif
    // re-use encoder handler - only select UI item, don't increment
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
    case SEQ_UI_BUTTON_GP16: {
      if( SEQ_GROOVE_Clear(SEQ_CC_Get(visible_track, SEQ_CC_GROOVE_STYLE)) < 0 )
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Preset", "not editable!");
      else
	SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "Groove", "cleared!");
    } break;

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
  // Trk.  Groove Style  Intensity           Step Dly. Len. Vel. NumSteps        Clr 
  // G1T1  Inv. Shuffle     15                 1    0    0    0  Preset not editable!


  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 selected_groove = SEQ_CC_Get(visible_track, SEQ_CC_GROOVE_STYLE);
  seq_groove_entry_t *g;
  if( selected_groove >= SEQ_GROOVE_NUM_PRESETS )
    g = (seq_groove_entry_t *)&seq_groove_templates[selected_groove-SEQ_GROOVE_NUM_PRESETS];
  else
    g = (seq_groove_entry_t *)&seq_groove_presets[selected_groove];


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Trk.  Groove Style  Intensity           Step Dly. Len. Vel. NumSteps        Clr ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_STYLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(12);
  } else {
    SEQ_LCD_PrintString(SEQ_GROOVE_NameGet(selected_groove));
  }
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_VALUE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_GROOVE_VALUE));
  }

  SEQ_LCD_PrintSpaces(14);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_STEP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString(" %2d ", edit_step+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_DELAY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    int value = g->add_step_delay[edit_step];
    if( value <= -128 )
      SEQ_LCD_PrintString("VNEG ");
    else if( value >= 127 )
      SEQ_LCD_PrintString("VPOS ");
    else
      SEQ_LCD_PrintFormattedString("%4d ", value);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    int value = g->add_step_length[edit_step];
    if( value <= -128 )
      SEQ_LCD_PrintString("VNEG ");
    else if( value >= 127 )
      SEQ_LCD_PrintString("VPOS ");
    else
      SEQ_LCD_PrintFormattedString("%4d ", value);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GROOVE_VELOCITY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    int value = g->add_step_velocity[edit_step];
    if( value <= -128 )
      SEQ_LCD_PrintString("VNEG ");
    else if( value >= 127 )
      SEQ_LCD_PrintString("VPOS ");
    else
      SEQ_LCD_PrintFormattedString("%4d ", value);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( selected_groove >= SEQ_GROOVE_NUM_PRESETS ) {
    if( ui_selected_item == ITEM_GROOVE_NUM_STEPS && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(6);
    } else {
      SEQ_LCD_PrintFormattedString("  %2d  ", g->num_steps);
    }    
    SEQ_LCD_PrintSpaces(15);
  } else {
    if( ui_cursor_flash && ui_selected_item >= ITEM_GROOVE_LENGTH && ui_selected_item <= ITEM_GROOVE_DELAY )
      SEQ_LCD_PrintSpaces(21);
    else
      SEQ_LCD_PrintString(" Preset not editable!");
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_G_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    store_file_required = 0;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKGRV_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  store_file_required = 0;

  return 0; // no error
}
