// $Id: seq_ui_trkmode.c 1142 2011-02-17 23:19:36Z tk $
/*
 * Track mode page
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
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       8
#define ITEM_GXTY          0
#define ITEM_MODE          1
#define ITEM_BUS           2
#define ITEM_HOLD          3
#define ITEM_SORT          4
#define ITEM_RESTART       5
#define ITEM_FORCE_SCALE   6
#define ITEM_SUSTAIN       7


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;
/*
  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_MODE: {
      int selected_mode = SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_MODE);
      *gp_leds = (1 << (selected_mode+1));
      }
      break;
    case ITEM_BUS:         *gp_leds = 0x0080; break;
    case ITEM_HOLD:        *gp_leds = 0x0100; break;
    case ITEM_SORT:        *gp_leds = 0x0200; break;
    case ITEM_RESTART:     *gp_leds = 0x0c00; break;
    case ITEM_FORCE_SCALE: *gp_leds = 0x3000; break;
    case ITEM_SUSTAIN:     *gp_leds = 0xc000; break;
  }
*/
    *gp_leds = 0x0000;
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
		if( SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 1 )
			return 1;
		else
			return 0;
      break; 


//	ui_selected_item = ITEM_GXTY;
//      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_MODE;
      SEQ_UI_CC_Set(SEQ_CC_MODE, encoder-1);
      return 1;

    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_BUS;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_HOLD;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_SORT;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_RESTART;
      break;

    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_FORCE_SCALE;
      break;

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_SUSTAIN;
      break;
  }

  // for GP encoders and Datawheel
  if (encoder == SEQ_UI_ENCODER_Datawheel){  
  switch( ui_selected_item ) {
    case ITEM_GXTY:
      return SEQ_UI_GxTyInc(incrementer);

    case ITEM_MODE:
      return SEQ_UI_CC_Inc(SEQ_CC_MODE, 0, 3, incrementer);

    case ITEM_BUS:
      return SEQ_UI_CC_Inc(SEQ_CC_BUSASG, 0, 3, incrementer);

    case ITEM_HOLD:
      if( !incrementer ) // toggle flag
	incrementer = (SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1<<1)) ? -1 : 1;
      return SEQ_UI_CC_SetFlags(SEQ_CC_MODE_FLAGS, (1<<1), (incrementer >= 0) ? (1<<1) : 0);

    case ITEM_SORT:
      if( !incrementer ) // toggle flag - SORT is inverted!
	incrementer = (SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1<<0)) ? 1 : -1;
      return SEQ_UI_CC_SetFlags(SEQ_CC_MODE_FLAGS, (1<<0), (incrementer >= 0) ? 0 : (1<<0)); // SORT is inverted!

    case ITEM_RESTART:
      if( !incrementer ) // toggle flag
	incrementer = (SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1<<2)) ? -1 : 1;
      return SEQ_UI_CC_SetFlags(SEQ_CC_MODE_FLAGS, (1<<2), (incrementer >= 0) ? (1<<2) : 0);

    case ITEM_FORCE_SCALE:
      if( !incrementer ) // toggle flag
	incrementer = (SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1<<3)) ? -1 : 1;
      return SEQ_UI_CC_SetFlags(SEQ_CC_MODE_FLAGS, (1<<3), (incrementer >= 0) ? (1<<3) : 0);

    case ITEM_SUSTAIN:
      if( !incrementer ) // toggle flag
	incrementer = (SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1<<4)) ? -1 : 1;
      return SEQ_UI_CC_SetFlags(SEQ_CC_MODE_FLAGS, (1<<4), (incrementer >= 0) ? (1<<4) : 0);
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
  // Trk. off     Transpose              Bus Hold Sort  Restart  ForceScale  Sustain 
  // G1T1   >Normal<  Arpeggiator         1   on   on     on        on         on   

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  ///////////////////////////////////////////////////////////////////////////  
  //SEQ_LCD_PrintString("Trk.");
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
	
	if( ui_selected_item == ITEM_GXTY ) {		
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("TRACK ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
				
    } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		SEQ_LCD_PrintString("TRACK ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}
	SEQ_LCD_PrintString("MODE      ");	
	
  ///////////////////////////////////////////////////////////////////////////
  //SEQ_LCD_CursorSet(0, 1);
  ///////////////////////////////////////////////////////////////////////////
/*  
  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(9);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(5);
  }
*/
    //SEQ_LCD_PrintSpaces(21);
  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_MODE ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Mode   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Mode   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }
/*
  const char mode_names[7][14] = {
    ">off<        ",
    ">Normal<     ",
    ">Transpose<  ",
    ">Arpeggiator<"
  };
*/ 
  //int i;
  int selected_mode = SEQ_CC_Get(visible_track, SEQ_CC_MODE);
	  switch( selected_mode ) {
		case 0:
			 SEQ_LCD_PrintString("off           ");
			break;
		case 1:
			 SEQ_LCD_PrintString("Normal        ");
			break;
		case 2:
			 SEQ_LCD_PrintString("Transpose     ");
			break;
		case 3:
			 SEQ_LCD_PrintString("Arpeggiator   ");
			break;
		}  
/*  
  for(i=0; i<4; ++i) {
    u8 x = 0 + 5*i;
    SEQ_LCD_CursorSet(x, (1+i%2));

    // print unmodified name if item selected
    // replace '>' and '<' by space if item not selected
    // flash item (print only '>'/'<' and spaces) if cursor position == 1 and flash flag set by timer
    int j;
    for(j=0; j<13; ++j) {
      u8 c = mode_names[i][j];

      if( ++x > 21 ) // don't print more than 35 characters per line
	break;

      if( c == '>' || c == '<' ) {
	SEQ_LCD_PrintChar((i == selected_mode) ? c : ' ');
      } else {
	if( ui_selected_item == ITEM_MODE && i == selected_mode && ui_cursor_flash )
	  SEQ_LCD_PrintChar(' ');
	else
	  SEQ_LCD_PrintChar(c);
      }
    }
  }
*/
  // additional spaces to fill LCD (avoids artefacts on page switches)
  //SEQ_LCD_CursorSet(25, 0);
  //SEQ_LCD_PrintSpaces(15);
  //SEQ_LCD_CursorSet(32, 1);
  //SEQ_LCD_PrintSpaces(8);

  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 2);  
  ///////////////////////////////////////////////////////////////////////////
  //SEQ_LCD_CursorSet(35, 0);
  if( ui_selected_item == ITEM_BUS ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Bus    : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Bus    : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }   
  
  //SEQ_LCD_PrintString(" Bus Hold Sort  Restart  ForceScale  Sustain ");
//  SEQ_LCD_CursorSet(35, 1);


//  if( ui_selected_item == ITEM_BUS && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintFormattedString("  %d", SEQ_CC_Get(visible_track, SEQ_CC_BUSASG) + 1);
// }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 3);
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_HOLD ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Hold   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Hold   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }

//  if( ui_selected_item == ITEM_HOLD && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1 << 1)) ? "on " : "off");
//  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 4);  
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_SORT ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Sort   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Sort   : ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }
  
//  if( ui_selected_item == ITEM_SORT && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1 << 0)) ? "off" : "on "); // SORT is inverted!
//  }
  SEQ_LCD_PrintSpaces(12);

  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 5);  
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_RESTART ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Restart: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Restart: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }  
  
//  if( ui_selected_item == ITEM_RESTART && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1 << 2)) ? "on " : "off");
//  }
  SEQ_LCD_PrintSpaces(9);

  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 6);  
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_FORCE_SCALE ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("ForceScale: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("ForceScale: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }
  
//  if( ui_selected_item == ITEM_FORCE_SCALE && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1 << 3)) ? "on " : "off");
//  }
  SEQ_LCD_PrintSpaces(6);

  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 7);  
  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_SUSTAIN ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Sustain: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintString("Sustain: ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }
  
//  if( ui_selected_item == ITEM_SUSTAIN && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(3);
//  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_MODE_FLAGS) & (1 << 4)) ? "on " : "off");
//  }
  SEQ_LCD_PrintSpaces(9);
  
/*  
  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 7);  
  ///////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_PrintSpaces(21);  
*/
  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKMODE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
