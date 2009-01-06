// $Id$
/*
 * Pattern page
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

#include "seq_file_b.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4
#define ITEM_PATTERN_G1    0
#define ITEM_PATTERN_G2    1
#define ITEM_PATTERN_G3    2
#define ITEM_PATTERN_G4    3


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static seq_pattern_t selected_pattern[SEQ_CORE_NUM_GROUPS];


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  seq_pattern_t pattern = (ui_selected_item == (ITEM_PATTERN_G1 + ui_selected_group) && ui_cursor_flash) 
    ? seq_pattern[ui_selected_group] : selected_pattern[ui_selected_group];

  *gp_leds = (1 << pattern.group) | (1 << (pattern.num+8));

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
      ui_selected_item = ITEM_PATTERN_G1;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_PATTERN_G1;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_PATTERN_G2;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_PATTERN_G2;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_PATTERN_G3;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_PATTERN_G3;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_PATTERN_G4;
      break;

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_PATTERN_G4;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped

    case SEQ_UI_ENCODER_Datawheel:
      return -1; // not mapped yet

    default:
      return -1; // invalid or unsupported encoder
  }

  // take over new group
  ui_selected_group = ui_selected_item;

  // change bank/pattern number
  seq_pattern_t *pattern = &selected_pattern[ui_selected_group];
  if( encoder & 1 ) {
    u8 tmp = pattern->pattern;
    u8 max_patterns = SEQ_FILE_B_NumPatterns(pattern->bank);
    // TODO: print error message if bank not valid (max_patterns = 0)
    if( !max_patterns || !SEQ_UI_Var8_Inc(&tmp, 0, max_patterns-1, incrementer) ) {
      return 0; // no change
    }
    pattern->pattern = tmp;
  } else {
    u8 tmp = pattern->bank;
    if( !SEQ_UI_Var8_Inc(&tmp, 0, SEQ_FILE_B_NUM_BANKS-1, incrementer) ) {
      return 0; // no change
    }
    pattern->bank = tmp;
  }

  SEQ_PATTERN_Change(ui_selected_group, *pattern);

  return 1; // value as been changed
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
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP8 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP8 ) {
#endif
    if( selected_pattern[ui_selected_group].group == button )
      selected_pattern[ui_selected_group].lower ^= 1;
    else
      selected_pattern[ui_selected_group].group = button;
    return 1; // value always changed
  }

  if( button >= SEQ_UI_BUTTON_GP9 && button <= SEQ_UI_BUTTON_GP16 ) {
    selected_pattern[ui_selected_group].num = button-8;
    SEQ_PATTERN_Change(ui_selected_group, selected_pattern[ui_selected_group]);

    return 1; // value always changed
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
  // take over selected group -> item
  ui_selected_item = ui_selected_group;

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //  >G1<       G2        G3        G4                                              
  // 1:A1 ____ 1:C1 ____ 1:E1 ____ 1:G1 ____                                         

  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters
    MIOS32_LCD_DeviceSet(0);

    u8 track;
    seq_core_trk_t *t = &seq_core_trk[0];
    for(track=0; track<16; ++t, ++track) {
      if( !(track % 4) )
	MIOS32_LCD_CursorSet(5 + 10*(track>>2), 1);

      SEQ_LCD_PrintVBar(t->vu_meter >> 4);
    }
  } else {
    ///////////////////////////////////////////////////////////////////////////
    MIOS32_LCD_DeviceSet(0);

    u8 group;
    for(group=0; group<4; ++group) {
      MIOS32_LCD_CursorSet(10*group, 0);
      MIOS32_LCD_PrintFormattedString((ui_selected_item == group) ? " >G%d<" : "  G%d ", group+1);
      SEQ_LCD_PrintSpaces(5);

      MIOS32_LCD_CursorSet(10*group, 1);
      // shortly show current pattern
      seq_pattern_t pattern = (ui_selected_item == (ITEM_PATTERN_G1 + group) && ui_cursor_flash) ? seq_pattern[group] : selected_pattern[group];
      MIOS32_LCD_PrintFormattedString("%d:", pattern.bank + 1);

      if( pattern.pattern < SEQ_FILE_B_NumPatterns(pattern.bank) )
	SEQ_LCD_PrintPattern(pattern);
      else
	MIOS32_LCD_PrintString("!!"); // covers the case that bank is not available, or that pattern number too high

      if( selected_pattern[group].pattern != seq_pattern[group].pattern ||
	  selected_pattern[group].bank != seq_pattern[group].bank )
	MIOS32_LCD_PrintChar('*');
      else
	MIOS32_LCD_PrintChar(' ');
      MIOS32_LCD_CursorSet(10*group + 9, 1);
      MIOS32_LCD_PrintChar(' ');
    }

    // second LCD currently unused - will change once we have more tracks
    MIOS32_LCD_DeviceSet(1);
    MIOS32_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintSpaces(40);
    MIOS32_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintSpaces(40);
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_PATTERN_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show vertical VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);

  // init selection patterns
  u8 group;
  for(group=0; group<SEQ_CORE_NUM_GROUPS; ++group)
    selected_pattern[group] = seq_pattern[group];

  return 0; // no error
}
