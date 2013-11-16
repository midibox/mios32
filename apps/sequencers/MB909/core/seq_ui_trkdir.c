// $Id: seq_ui_trkdir.c 1142 2011-02-17 23:19:36Z tk $
/*
 * Track direction page
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

#define NUM_OF_ITEMS       9
#define ITEM_GXTY          0
#define ITEM_DIRECTION     1
#define ITEM_STEPS_FORWARD 2
#define ITEM_STEPS_JMPBCK  3
#define ITEM_STEPS_REPLAY  4
#define ITEM_STEPS_REPEAT  5
#define ITEM_STEPS_SKIP    6
#define ITEM_STEPS_RS_INTERVAL 7
#define ITEM_SYNCH_TO_MEASURE 8


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_DIRECTION: {
      int selected_direction = SEQ_CC_Get(SEQ_UI_VisibleTrackGet(), SEQ_CC_DIRECTION);
      *gp_leds = (1 << (selected_direction+1));
      }
      break;
    case ITEM_STEPS_FORWARD:    *gp_leds = 0x0100; break;
    case ITEM_STEPS_JMPBCK:     *gp_leds = 0x0200; break;
    case ITEM_STEPS_REPLAY:     *gp_leds = 0x0c00; break;
    case ITEM_STEPS_REPEAT:     *gp_leds = 0x1000; break;
    case ITEM_STEPS_SKIP:       *gp_leds = 0x2000; break;
    case ITEM_STEPS_RS_INTERVAL: *gp_leds = 0x4000; break;
    case ITEM_SYNCH_TO_MEASURE: *gp_leds = 0x8000; break;
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
		SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS, incrementer);
		if (ui_selected_item == ITEM_DIRECTION) {
			//SEQ_UI_CC_Set(SEQ_CC_DIRECTION, encoder-1);
			SEQ_UI_CC_Set(SEQ_CC_DIRECTION, 7);
			SEQ_LCD_Update(1);
		}
      //ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_DIRECTION;
      //SEQ_UI_CC_Set(SEQ_CC_DIRECTION, encoder-1);
      return 1;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_STEPS_FORWARD;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_STEPS_JMPBCK;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_STEPS_REPLAY;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_STEPS_REPEAT;
      break;

    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_STEPS_SKIP;
      break;

    case SEQ_UI_ENCODER_GP15:
      ui_selected_item = ITEM_STEPS_RS_INTERVAL;
      break;

    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_SYNCH_TO_MEASURE;
      break;

  }

  
  // for GP encoders and Datawheel
  if (encoder == SEQ_UI_ENCODER_Datawheel) {
  switch( ui_selected_item ) {
    case ITEM_GXTY:         return SEQ_UI_GxTyInc(incrementer);
    case ITEM_DIRECTION:     return SEQ_UI_CC_Inc(SEQ_CC_DIRECTION, 0, 6, incrementer);
    case ITEM_STEPS_FORWARD: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_FORWARD, 0, 7, incrementer);
    case ITEM_STEPS_JMPBCK:  return SEQ_UI_CC_Inc(SEQ_CC_STEPS_JMPBCK, 0, 7, incrementer);
    case ITEM_STEPS_REPLAY:  return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPLAY, 0, 7, incrementer);
    case ITEM_STEPS_REPEAT:  return SEQ_UI_CC_Inc(SEQ_CC_STEPS_REPEAT, 0, 16, incrementer);
    case ITEM_STEPS_SKIP:    return SEQ_UI_CC_Inc(SEQ_CC_STEPS_SKIP, 0, 16, incrementer);
    case ITEM_STEPS_RS_INTERVAL: return SEQ_UI_CC_Inc(SEQ_CC_STEPS_RS_INTERVAL, 0, 63, incrementer);
    case ITEM_SYNCH_TO_MEASURE: return SEQ_UI_CC_SetFlags(SEQ_CC_CLKDIV_FLAGS, (1<<0), (incrementer >= 0) ? (1<<0) : 0);
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

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
    case SEQ_UI_BUTTON_GP6:
    case SEQ_UI_BUTTON_GP7:
    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_DIRECTION;
      SEQ_UI_CC_Set(SEQ_CC_DIRECTION, button-1);
      return 1; // value changed

    case SEQ_UI_BUTTON_GP9:
      ui_selected_item = ITEM_STEPS_FORWARD;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP10:
      ui_selected_item = ITEM_STEPS_JMPBCK;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
      ui_selected_item = ITEM_STEPS_REPLAY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP13:
      ui_selected_item = ITEM_STEPS_REPEAT;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP14:
      ui_selected_item = ITEM_STEPS_SKIP;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP15:
      ui_selected_item = ITEM_STEPS_RS_INTERVAL;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP16:
      return Encoder_Handler((int)button, (SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<0)) ? -1 : 1); // toggle flag
      return 0; // value hasn't been changed

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
  // >Forward<  PingPong  Rand.Dir  Rand.D+S Fwd. Back Replay  Repeat Skip Itv. SyncM
  // G1T1   Backward  Pendulum  Rand.Step      1    0    x1        0    0   16   on

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  if( ui_selected_item == ITEM_GXTY ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(2);
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(2);
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  }

  ///////////////////////////////////////////////////////////////////////////
  const char dir_names[7][13] = {
    ">Forward< ",
    ">Backward<",
    ">PingPong<",
    ">Pendulum<",
    ">Rand.Dir<",
    ">Ran.Step<",
    ">Rand.D+S< "
  };
  int i;
  int selected_direction = SEQ_CC_Get(visible_track, SEQ_CC_DIRECTION);
  if( ui_selected_item == ITEM_DIRECTION && 0 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString("     Forward ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString("     Forward ");
	  
  }	
	
	SEQ_LCD_CursorSet(0, 1);  
   if( ui_selected_item == ITEM_DIRECTION && 1 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" Backward ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" Backward ");
	  
  }	 
   if( ui_selected_item == ITEM_DIRECTION && 2 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" PingPong ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" PingPong ");
	  
  }	  
  
  
	SEQ_LCD_CursorSet(0, 2);    
    if( ui_selected_item == ITEM_DIRECTION && 3 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" Pendulum ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" Pendulum ");
	  
  }	 
   if( ui_selected_item == ITEM_DIRECTION && 4 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" Rand.Dir ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" Rand.Dir ");
	  
  }	   
 
	SEQ_LCD_CursorSet(0, 3);    
    if( ui_selected_item == ITEM_DIRECTION && 5 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" Ran.Step ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" Ran.Step ");
	  
  }	 
   if( ui_selected_item == ITEM_DIRECTION && 6 == selected_direction ){
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
	  SEQ_LCD_PrintString(" Rand.D+S ");
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}else{
	  SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	  SEQ_LCD_PrintString(" Rand.D+S ");
	  
  }	   

 /*
  for(i=0; i<7; ++i) {
    u8 x = 5*i;
    SEQ_LCD_CursorSet(x, i%2);

    // print unmodified name if item selected
    // replace '>' and '<' by space if item not selected
    // flash item (print only '>'/'<' and spaces) if cursor position == 1 and flash flag set by timer
    int j;
    for(j=0; j<10; ++j) {
      u8 c = dir_names[i][j];

      if( ++x > 20 ) // don't print more than 40 characters per line
	break;

      if( c == '>' || c == '<' ) {
	SEQ_LCD_PrintChar((i == selected_direction) ? c : ' ');
      } else {
	if( ui_selected_item == ITEM_DIRECTION && i == selected_direction && ui_cursor_flash )
	  SEQ_LCD_PrintChar(' ');
	else
	  SEQ_LCD_PrintChar(c);
      }
    }
  }
*/
  // 3 additional spaces to fill LCD (avoids artefacts on page switches)
  //SEQ_LCD_CursorSet(37, 1);
  //SEQ_LCD_PrintSpaces(3);


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 4);
   if( ui_selected_item == ITEM_STEPS_FORWARD ) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Fwd ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		
    } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		SEQ_LCD_PrintString("Fwd ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}
	
	if( ui_selected_item == ITEM_STEPS_JMPBCK ) {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Back ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		
    } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		SEQ_LCD_PrintString("Back ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}
		
	if( ui_selected_item == ITEM_STEPS_REPLAY ) {		
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Rplay ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		
    } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		SEQ_LCD_PrintString("Rplay ");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}		

	if( ui_selected_item == ITEM_STEPS_REPEAT ) {		
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
		SEQ_LCD_PrintString("Repeat");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		
    } else {
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);	
		SEQ_LCD_PrintString("Repeat");
		SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);		
	}			

		
  
   
/////////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 5);
//  if( ui_selected_item == ITEM_STEPS_FORWARD && ui_cursor_flash ) {
  //  SEQ_LCD_PrintSpaces(3);
 // } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_FORWARD)+1);
 // }
  SEQ_LCD_PrintSpaces(3);

 // if( ui_selected_item == ITEM_STEPS_JMPBCK && ui_cursor_flash ) {
  //  SEQ_LCD_PrintSpaces(3);
 // } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_JMPBCK));
 // }
  SEQ_LCD_PrintSpaces(3);

 // if( ui_selected_item == ITEM_STEPS_REPLAY && ui_cursor_flash ) {
  //  SEQ_LCD_PrintSpaces(3);
  //} else {
    int value = SEQ_CC_Get(visible_track, SEQ_CC_STEPS_REPLAY)+1;
    SEQ_LCD_PrintFormattedString("x%d", value);
    if( value < 10 )
      SEQ_LCD_PrintSpaces(1);
 // }  
  SEQ_LCD_PrintSpaces(3);

  //if( ui_selected_item == ITEM_STEPS_REPEAT && ui_cursor_flash ) {
    //SEQ_LCD_PrintSpaces(3);
  //} else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_REPEAT));
  //}
  SEQ_LCD_PrintSpaces(2);

/////////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 6);
  SEQ_LCD_PrintString("  Skip Itv. SyncM    ");
  
/////////////////////////////////////////////////////////////////////////////  
  SEQ_LCD_CursorSet(0, 7);
  if( ui_selected_item == ITEM_STEPS_SKIP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_SKIP));
  }
  SEQ_LCD_PrintSpaces(2);

  if( ui_selected_item == ITEM_STEPS_RS_INTERVAL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_STEPS_RS_INTERVAL)+1);
  }
  SEQ_LCD_PrintSpaces(4);

  if( ui_selected_item == ITEM_SYNCH_TO_MEASURE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString((SEQ_CC_Get(visible_track, SEQ_CC_CLKDIV_FLAGS) & (1<<0)) ? "yes" : "no ");
  }
  SEQ_LCD_PrintSpaces(1);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKDIR_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
