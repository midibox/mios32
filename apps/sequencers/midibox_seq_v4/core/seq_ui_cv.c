// $Id$
/*
 * CV configuration page
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
#include <aout.h>
#include "tasks.h"

#include "seq_lcd.h"
#include "seq_ui.h"

#include "seq_file_c.h"
#include "seq_file_gc.h"

#include "seq_cv.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       10
#define ITEM_CV            0
#define ITEM_CURVE         1
#define ITEM_SLEWRATE      2
#define ITEM_PITCHRANGE    3
#define ITEM_GATE          4
#define ITEM_CALIBRATION   5
#define ITEM_CLK_SEL       6
#define ITEM_CLK_PPQN      7
#define ITEM_CLK_WIDTH     8
#define ITEM_MODULE        9

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 store_file_required;
static u8 selected_cv;
static u8 selected_clkout;

const u16 din_sync_div_presets[] =
  // 1    2    3    4   6   8  12  16  24  32  48  96  192  384 ppqn, StartStop(=0)
  { 384, 192, 128, 96, 64, 48, 32, 24, 16, 12,  8,  4,   2,  1, 0 };



/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_CV:          *gp_leds = 0x0001; break;
    case ITEM_CURVE:       *gp_leds = 0x0002; break;
    case ITEM_SLEWRATE:    *gp_leds = 0x000c; break;
    case ITEM_PITCHRANGE:  *gp_leds = 0x0010; break;
    case ITEM_GATE:        *gp_leds = 0x0020; break;
    case ITEM_CALIBRATION: *gp_leds = 0x00c0; break;
    case ITEM_CLK_SEL:     *gp_leds = 0x0100; break;
    case ITEM_CLK_PPQN:    *gp_leds = 0x0600; break;
    case ITEM_CLK_WIDTH:   *gp_leds = 0x0800; break;
    case ITEM_MODULE:      *gp_leds = 0xc000; break;
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
      ui_selected_item = ITEM_CV;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_CURVE;
      break;

    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_SLEWRATE;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_PITCHRANGE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_GATE;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_CALIBRATION;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_CLK_SEL;
      break;

    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_CLK_PPQN;
      break;

    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_CLK_WIDTH;
      break;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_MODULE;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_CV:
      if( SEQ_UI_Var8_Inc(&selected_cv, 0, SEQ_CV_NUM-1, incrementer) >= 0 ) {
	SEQ_CV_CaliModeSet(selected_cv, SEQ_CV_CaliModeGet()); // change calibration mode to new pin
	return 1;
      }
      return 0;

    case ITEM_CURVE: {
    u8 curve = SEQ_CV_CurveGet(selected_cv);
      if( SEQ_UI_Var8_Inc(&curve, 0, SEQ_CV_NUM_CURVES-1, incrementer) >= 0 ) {
	SEQ_CV_CurveSet(selected_cv, curve);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_SLEWRATE: {
      u8 rate = SEQ_CV_SlewRateGet(selected_cv);
      if( SEQ_UI_Var8_Inc(&rate, 0, 255, incrementer) >= 0 ) {
	SEQ_CV_SlewRateSet(selected_cv, rate);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_PITCHRANGE: {
      u8 range = SEQ_CV_PitchRangeGet(selected_cv);
      if( SEQ_UI_Var8_Inc(&range, 0, 127, incrementer) >= 0 ) {
	SEQ_CV_PitchRangeSet(selected_cv, range);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_GATE: {
      u8 inv = SEQ_CV_GateInversionGet(selected_cv);
      if( SEQ_UI_Var8_Inc(&inv, 0, 1, incrementer) >= 0 ) {
	SEQ_CV_GateInversionSet(selected_cv, inv);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_CALIBRATION: {
      u8 mode = SEQ_CV_CaliModeGet();
      if( SEQ_UI_Var8_Inc(&mode, 0, SEQ_CV_NUM_CALI_MODES-1, incrementer) >= 0 ) {
	SEQ_CV_CaliModeSet(selected_cv, mode);
	store_file_required = 1;
	return 1;
      }
      return 0;
    }

    case ITEM_CLK_SEL:
      if( SEQ_UI_Var8_Inc(&selected_clkout, 0, SEQ_CV_NUM_CLKOUT-1, incrementer) >= 0 ) {
	return 1;
      }
      return 0;

    case ITEM_CLK_PPQN: {
      int i;
      u8 din_sync_div_ix = 0;
      u16 div = SEQ_CV_ClkDividerGet(selected_clkout);

      for(i=0; i<sizeof(din_sync_div_presets)/sizeof(u16); ++i)
	if( div == din_sync_div_presets[i] ) {
	  din_sync_div_ix = i;
	  break;
	}

      if( SEQ_UI_Var8_Inc(&din_sync_div_ix, 0, (sizeof(din_sync_div_presets)/sizeof(u16))-1, incrementer) ) {
	SEQ_CV_ClkDividerSet(selected_clkout, din_sync_div_presets[din_sync_div_ix]);
	store_file_required = 1;
	return 1; // value has been changed
      } else
	return 0; // value hasn't been changed
    } break;

    case ITEM_CLK_WIDTH: {
      u8 width = SEQ_CV_ClkPulseWidthGet(selected_clkout);
      if( SEQ_UI_Var8_Inc(&width, 1, 255, incrementer) >= 0 ) {
	SEQ_CV_ClkPulseWidthSet(selected_clkout, width);
	store_file_required = 1;
	return 1;
      }
      return 0;
    } break;

    case ITEM_MODULE: {
      u8 if_type = SEQ_CV_IfGet();
      if( SEQ_UI_Var8_Inc(&if_type, 0, SEQ_CV_NUM_IF-1, incrementer) >= 0 ) {
	SEQ_CV_IfSet(if_type);
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

#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // re-use encoder handler - only select UI item, don't increment, flags will be toggled
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
  switch( button ) {
    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( depressed ) return -1;
      if( ++ui_selected_item >= NUM_OF_ITEMS )
	ui_selected_item = 0;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      if( ui_selected_item == 0 )
	ui_selected_item = NUM_OF_ITEMS-1;
      else
	--ui_selected_item;
      return 1; // value always changed

    case SEQ_UI_BUTTON_Up:
      if( depressed ) return -1;
      return Encoder_Handler(SEQ_UI_ENCODER_Datawheel, 1);

    case SEQ_UI_BUTTON_Down:
      if( depressed ) return -1;
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
  //  CV  Curve SlewRate PRng  Gate  Calibr.  Clk   Rate    Width             Module 
  //   1  V/Oct    0 mS    2   Pos.    off     1   24 PPQN   1 mS             AOUT_NG


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString(" CV  Curve SlewRate PRng  Gate  Calibr.  Clk   Rate    Width             Module ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CV && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString(" %2d  ", selected_cv+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CURVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintString((char *)SEQ_CV_CurveNameGet(selected_cv));
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_SLEWRATE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(9);
  } else {
    SEQ_LCD_PrintFormattedString(" %3d mS  ", SEQ_CV_SlewRateGet(selected_cv));
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PITCHRANGE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintFormattedString("%3d   ", SEQ_CV_PitchRangeGet(selected_cv));
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_GATE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintFormattedString(SEQ_CV_GateInversionGet(selected_cv) ? "Neg.  " : "Pos.  ");
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CALIBRATION && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(8);
  } else {
    SEQ_LCD_PrintFormattedString(" %s ", SEQ_CV_CaliNameGet());
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CLK_SEL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintFormattedString("%3d ", selected_clkout + 1);
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CLK_PPQN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(9);
  } else {
    u16 divider = SEQ_CV_ClkDividerGet(selected_clkout);
    if( !divider ) {
      SEQ_LCD_PrintFormattedString("StartStop", 384 / divider);
    } else {
      SEQ_LCD_PrintFormattedString("%3d PPQN ", 384 / divider);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CLK_WIDTH && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(6);
  } else {
    SEQ_LCD_PrintFormattedString("%3d mS", SEQ_CV_ClkPulseWidthGet(selected_clkout));
  }

  SEQ_LCD_PrintSpaces(12);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MODULE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(8);
  } else {
    SEQ_LCD_PrintString((char *)SEQ_CV_IfNameGet(SEQ_CV_IfGet()));
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  // ensure that calibration mode disabled
  SEQ_CV_CaliModeSet(selected_cv, AOUT_CALI_MODE_OFF);

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    store_file_required = 0;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_CV_Init(u32 mode)
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
