// $Id$
/*
 * BPM configuration page
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
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file_c.h"

#include "seq_cc.h"
#include "seq_bpm.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       4
#define ITEM_MODE          0
#define ITEM_BPM           1
#define ITEM_IDIV          2
#define ITEM_EDIV          3


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_MODE: *gp_leds = 0x0007; break;
    case ITEM_BPM: *gp_leds = 0x0078; break;
  }

  *gp_leds |= (1 << (seq_core_bpm_div_int+8));
  *gp_leds |= (1 << (seq_core_bpm_div_ext+12));

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
    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_MODE;
      break;

    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_BPM;
      // special feature: these two encoders increment *10
      incrementer *= 10;
      break;

    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_BPM;
      break;

    case SEQ_UI_ENCODER_GP8:
      return 0; // Tap Tempo not for encoder

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_IDIV;
      seq_core_bpm_div_int = (u8)encoder & 3;
      return 1;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_EDIV;
      seq_core_bpm_div_ext = (u8)encoder & 3;
      return 1;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_MODE: {
      u8 value = SEQ_BPM_ModeGet();
      if( SEQ_UI_Var8_Inc(&value, 0, 2, incrementer) ) {
	SEQ_BPM_ModeSet(value);
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_BPM: {
      u16 value = (u16)(SEQ_BPM_Get()*10);
      if( SEQ_UI_Var16_Inc(&value, 25, 3000, incrementer) ) { // at 384ppqn, the minimum BPM rate is ca. 2.5
	SEQ_BPM_Set((float)value/10.0);
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_IDIV:
      return SEQ_UI_Var8_Inc(&seq_core_bpm_div_int, 0, 3, incrementer);

    case ITEM_EDIV:
      return SEQ_UI_Var8_Inc(&seq_core_bpm_div_ext, 0, 3, incrementer);
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
    // re-use encoder handler - only select UI item, don't increment
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
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
  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // BPM Clock Mode   Beats per Minute   Tap    Global Clock Dividers (Int./Ext.)    
  //     Master            140.0        Tempo  >1<   2    4    8    2   >4<   8   16 


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("BPM Clock Mode   Beats per Minute   Tap    Global Clock Dividers (Int./Ext.)    ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintSpaces(4);
  if( ui_selected_item == ITEM_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    const char mode_str[3][7] = { "Auto  ", "Master", "Slave "};
    SEQ_LCD_PrintString((char *)mode_str[SEQ_BPM_ModeGet()]);
  }
  SEQ_LCD_PrintSpaces(11);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_BPM && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    float bpm = SEQ_BPM_Get();
    SEQ_LCD_PrintFormattedString("%3d.%d", (int)bpm, (int)(10*bpm)%10);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintSpaces(9);
  SEQ_LCD_PrintString("Tempo");

  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<4; ++i) {
    if( seq_core_bpm_div_int == i && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(5);
    else
      SEQ_LCD_PrintFormattedString("  %2d ", 1 << i);
  }

  for(i=0; i<4; ++i) {
    if( seq_core_bpm_div_ext == i && ui_cursor_flash )
      SEQ_LCD_PrintSpaces(5);
    else {
      SEQ_LCD_PrintFormattedString("  %2d ", 1 << (i+1));
    }
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status;

  // write config file
  MUTEX_SDCARD_TAKE;
  if( (status=SEQ_FILE_C_Write()) < 0 )
    SEQ_UI_SDCardErrMsg(2000, status);
  MUTEX_SDCARD_GIVE;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_BPM_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  return 0; // no error
}
