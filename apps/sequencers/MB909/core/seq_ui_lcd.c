// $Id: seq_ui_todo.c 278 2009-01-10 19:55:35Z tk $
/*
 * LCD page (contrast setup page for the DOGM LCD)
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

#include "seq_file.h"
#include "seq_file_gc.h"

#include "seq_lcd.h"
#include "seq_ui.h"
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

//static u8 store_file_required;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

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
  if (encoder == SEQ_UI_ENCODER_Datawheel){
	u8 value= SEQ_LCD_Contrastget();
	value= value-14;
	MIOS32_DELAY_Wait_uS(50000); 
	if (incrementer <= -1) 
		incrementer = -1;
	if (incrementer >= 1) 
		incrementer = 1;
	if( SEQ_UI_Var8_Inc(&value, 1, 32, incrementer) >= 1 ){


			 lcd_contrast = value+14;

			ui_store_file_required = 1;
			return 1;
		}else
			return 0;
	}
  return -1; // invalid or unsupported encoder
}


/////////////////////////////////////////////////////////////////////////////
// Local button callback function
// Should return:
//   1 if value has been changed
//   0 if value hasn't been changed
//  -1 if invalid or unsupported gp_button
/////////////////////////////////////////////////////////////////////////////
static s32 Button_Handler(seq_ui_button_t button, s32 depressed)
{
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
	
  // print TODO message
  //SEQ_LCD_CursorSet(0, 0);
  //SEQ_LCD_PrintString("LCD contrast setup   ");
  //SEQ_LCD_CursorSet(0, 1);
  //SEQ_LCD_PrintFormattedString("%02d", lcd_contrast);
  SEQ_LCD_Contrastset(lcd_contrast);
	
  SEQ_LCD_CursorSet(1, 0);
  //MIOS32_LCD_CursorSet(1, 0);
  //SEQ_LCD_FontInit((u8 *)GLCD_FONT_BIG);
  SEQ_LCD_PrintFormattedString("%2d", lcd_contrast-14);
  SEQ_LCD_CursorSet(1, 3);
  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
  SEQ_LCD_PrintString("contrast");
  //MIOS32_LCD_GCursorSet(4, 40);
  SEQ_LCD_CursorSet(1, 5);
  //SEQ_LCD_FontInit((u8 *)GLCD_FONT_BIG);	
  SEQ_LCD_PrintString("SETUP");
  //SEQ_LCD_CursorSet(1, 7); 
  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
		


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( ui_store_file_required ) {
    // write config files
    //MUTEX_SDCARD_TAKE;
   // if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
    //  SEQ_UI_SDCardErrMsg(2000, status);
   // MUTEX_SDCARD_GIVE;

    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

    ui_store_file_required = 0;
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_LCD_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

  ui_store_file_required = 0;
 // lcd_contrast = 0x10;
  return 0; // no error
}
