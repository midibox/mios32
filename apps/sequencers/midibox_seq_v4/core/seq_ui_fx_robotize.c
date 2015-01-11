// $Id$
/*
 * Fx Robotize page
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
#include "seq_robotize.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS                12
#define ITEM_GXTY                   0
#define ITEM_ROBOTIZE_ACTIVE	    1
#define ITEM_ROBOTIZE_PROBABILITY   2
#define ITEM_ROBOTIZE_SKIP          3
#define ITEM_ROBOTIZE_OCTAVE        4
#define ITEM_ROBOTIZE_NOTE          5
#define ITEM_ROBOTIZE_VELOCITY      6
#define ITEM_ROBOTIZE_LENGTH        7
#define ITEM_ROBOTIZE_SUSTAIN       8
#define ITEM_ROBOTIZE_NOFX			9
#define ITEM_ROBOTIZE_ECHO			10
#define ITEM_ROBOTIZE_DUPLICATE		11

/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

	if (  seq_ui_button_state.SELECT_PRESSED  ) { //edit robotize mask
		u8 visible_track = SEQ_UI_VisibleTrackGet();
		u16 robotize_mask = ( SEQ_CC_Get ( visible_track, SEQ_CC_ROBOTIZE_MASK2 ) << 8 ) | SEQ_CC_Get ( visible_track, SEQ_CC_ROBOTIZE_MASK1 );
		*gp_leds = robotize_mask;
		return 0;
	}//endif
		
  switch( ui_selected_item ) {
    case ITEM_GXTY: 				*gp_leds = 0x0001; break;
    case ITEM_ROBOTIZE_ACTIVE: 		*gp_leds = 0x0002; break;
    case ITEM_ROBOTIZE_PROBABILITY:	*gp_leds = 0x0004; break;
    case ITEM_ROBOTIZE_SKIP: 		*gp_leds = 0x0008; break;
    case ITEM_ROBOTIZE_OCTAVE: 		*gp_leds = 0x0010; break;
    case ITEM_ROBOTIZE_NOTE: 		*gp_leds = 0x0020; break;
    case ITEM_ROBOTIZE_VELOCITY: 	*gp_leds = 0x0040; break;
    case ITEM_ROBOTIZE_LENGTH: 		*gp_leds = 0x0080; break;
    case ITEM_ROBOTIZE_SUSTAIN: 	*gp_leds = 0x0100; break;
    case ITEM_ROBOTIZE_NOFX: 		*gp_leds = 0x0200; break;
    case ITEM_ROBOTIZE_ECHO: 		*gp_leds = 0x0400; break;
    case ITEM_ROBOTIZE_DUPLICATE: 	*gp_leds = 0x0800; break;
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
      ui_selected_item = ITEM_ROBOTIZE_ACTIVE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_ROBOTIZE_PROBABILITY;
      break;
    
    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_ROBOTIZE_SKIP;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_ROBOTIZE_OCTAVE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_ROBOTIZE_NOTE;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_ROBOTIZE_VELOCITY;
      break;
      
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_ROBOTIZE_LENGTH;
      break;

    case SEQ_UI_ENCODER_GP9:
      //ui_selected_item = ITEM_ROBOTIZE_SUSTAIN;
      return -1;
      break;
      
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_ROBOTIZE_NOFX;
      break;
      
    case SEQ_UI_ENCODER_GP11:
      //ui_selected_item = ITEM_ROBOTIZE_ECHO;
      return -1;
      break;
      
    case SEQ_UI_ENCODER_GP12:
      //ui_selected_item = ITEM_ROBOTIZE_DUPLICATE;
      return -1;
      break;
      
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not mapped
  }

  // for GP encoders and Datawheel
  if ( seq_ui_button_state.SELECT_PRESSED ){//adjust probability settings
		  switch( ui_selected_item ) {
			case ITEM_GXTY:              		return SEQ_UI_GxTyInc(incrementer);
			case ITEM_ROBOTIZE_ACTIVE:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_ACTIVE, 0, 1, incrementer);
			case ITEM_ROBOTIZE_PROBABILITY:   	return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_SKIP:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_SKIP_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_OCTAVE:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_OCT_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_NOTE:     		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_NOTE_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_VELOCITY: 		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_VEL_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_LENGTH:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_LEN_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_SUSTAIN:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_SUSTAIN_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_NOFX:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_NOFX_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_ECHO:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_ECHO_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_DUPLICATE:   	return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_DUPLICATE_PROBABILITY, 0, 31, incrementer);
	      }
	  } else {//adjust robotizer ranges
		  switch( ui_selected_item ) {
			case ITEM_GXTY:              		return SEQ_UI_GxTyInc(incrementer);
			case ITEM_ROBOTIZE_ACTIVE:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_ACTIVE, 0, 1, incrementer);
			case ITEM_ROBOTIZE_PROBABILITY:   	return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_SKIP:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_SKIP_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_OCTAVE:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_OCT, 0, 7, incrementer);
			case ITEM_ROBOTIZE_NOTE:     		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_NOTE, 0, 15, incrementer);
			case ITEM_ROBOTIZE_VELOCITY: 		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_VEL, 0, 127, incrementer);
			case ITEM_ROBOTIZE_LENGTH:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_LEN, 0, 127, incrementer);
			case ITEM_ROBOTIZE_SUSTAIN:   		return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_SUSTAIN_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_NOFX:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_NOFX_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_ECHO:   			return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_ECHO_PROBABILITY, 0, 31, incrementer);
			case ITEM_ROBOTIZE_DUPLICATE:   	return SEQ_UI_CC_Inc(SEQ_CC_ROBOTIZE_DUPLICATE_PROBABILITY, 0, 31, incrementer);
		  }
      }//endif

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


	if ( seq_ui_button_state.SELECT_PRESSED ) { //edit robotize mask
		u8 xormask = 0x00;
		u8 xormask2 = 0x00;
		
		switch( button ) {
			case SEQ_UI_BUTTON_GP1:
			  xormask = xormask | 0x01;
			  break;
			case SEQ_UI_BUTTON_GP2:
			  xormask = xormask | 0x02;
			  break;
			case SEQ_UI_BUTTON_GP3:
			  xormask = xormask | 0x04;
			  break;
			case SEQ_UI_BUTTON_GP4:
			  xormask = xormask | 0x08;
			  break;
			case SEQ_UI_BUTTON_GP5:
			  xormask = xormask | 0x10;
			  break;
			case SEQ_UI_BUTTON_GP6:
			  xormask = xormask | 0x20;
			  break;
			case SEQ_UI_BUTTON_GP7:
			  xormask = xormask | 0x40;
			  break;
			case SEQ_UI_BUTTON_GP8:
			  xormask = xormask | 0x80;
			  break;
			case SEQ_UI_BUTTON_GP9:
			  xormask2 = xormask2 | 0x01;
			  break;
			case SEQ_UI_BUTTON_GP10:
			  xormask2 = xormask2 | 0x02;
			  break;
			case SEQ_UI_BUTTON_GP11:
			  xormask2 = xormask2 | 0x04;
			  break;
			case SEQ_UI_BUTTON_GP12:
			  xormask2 = xormask2 | 0x08;
			  break;
			case SEQ_UI_BUTTON_GP13:
			  xormask2 = xormask2 | 0x10;
			  break;
			case SEQ_UI_BUTTON_GP14:
			  xormask2 = xormask2 | 0x20;
			  break;
			case SEQ_UI_BUTTON_GP15:
			  xormask2 = xormask2 | 0x40;
			  break;
			case SEQ_UI_BUTTON_GP16:
			  xormask2 = xormask2 | 0x80;
			  break;
		  }//endswitch
		u8 visible_track = SEQ_UI_VisibleTrackGet();
		SEQ_CC_Set(visible_track, SEQ_CC_ROBOTIZE_MASK1, SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_MASK1) ^ xormask);
		SEQ_CC_Set(visible_track, SEQ_CC_ROBOTIZE_MASK2, SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_MASK2) ^ xormask2);
		
		return 1;
	}//endif


#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    // re-use encoder handler - only select UI item, don't increment
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
  // Trk.  Intensity     Note Vel/CC Length  
  // G1T1      0          off   off   off    

    // we want to show vertical bars
    SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_VBars);
    
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Trk. Robot Prob Skip Octv Note VelCC Len");
//  SEQ_LCD_PrintString("Sust NoFX +Echo +Dup                    ");
    SEQ_LCD_PrintString("     NoFX                               ");
  
  
  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_ACTIVE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString( ( SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_ACTIVE) ) ? "  on" : " off" );
  }
  SEQ_LCD_PrintSpaces(2);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_PROBABILITY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_SKIP && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_SKIP_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_SKIP_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(1);





  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_OCTAVE && ui_cursor_flash &&  seq_ui_button_state.SELECT_PRESSED ) {
    SEQ_LCD_PrintSpaces(1);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_OCT));
  } else if( ui_selected_item == ITEM_ROBOTIZE_OCTAVE && ui_cursor_flash ) {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_OCT_PROBABILITY) >> 2);
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_OCT_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_OCT));    
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_NOTE && ui_cursor_flash  &&  seq_ui_button_state.SELECT_PRESSED ) {
    SEQ_LCD_PrintSpaces(1);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOTE));
  } else if( ui_selected_item == ITEM_ROBOTIZE_NOTE && ui_cursor_flash ) {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOTE_PROBABILITY) >> 2);
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOTE_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOTE));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_VELOCITY && ui_cursor_flash   &&  seq_ui_button_state.SELECT_PRESSED ) {
    SEQ_LCD_PrintSpaces(1);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_VEL));
  } else if( ui_selected_item == ITEM_ROBOTIZE_VELOCITY && ui_cursor_flash ) {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_VEL_PROBABILITY) >> 2);
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_VEL_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_VEL));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_ROBOTIZE_LENGTH && ui_cursor_flash   &&  seq_ui_button_state.SELECT_PRESSED  ) {
    SEQ_LCD_PrintSpaces(1);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_LEN));
  } else if( ui_selected_item == ITEM_ROBOTIZE_LENGTH && ui_cursor_flash ) {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_LEN_PROBABILITY) >> 2);
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_LEN_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_LEN));
  }
  SEQ_LCD_PrintSpaces(0);

///////////////////////////////////////////////////////////////////////////
// SECOND LCD -   SEQ_LCD_PrintString("Sust NoFX +Echo +Dup                    ");
///////////////////////////////////////////////////////////////////////////

/* UNDER DEVELOPMENT
 * 
  if( ui_selected_item == ITEM_ROBOTIZE_SUSTAIN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_SUSTAIN_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_SUSTAIN_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(1);

*/
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_ROBOTIZE_NOFX && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOFX_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_NOFX_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(2);

/* UNDER DEVELOPMENT
 * 
  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_ROBOTIZE_ECHO && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_ECHO_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_ECHO_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////

  if( ui_selected_item == ITEM_ROBOTIZE_DUPLICATE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintVBar(SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_DUPLICATE_PROBABILITY) >> 2);
    SEQ_LCD_PrintFormattedString("%3d", SEQ_CC_Get(visible_track, SEQ_CC_ROBOTIZE_DUPLICATE_PROBABILITY));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////

*/
  SEQ_LCD_PrintSpaces(11);



  SEQ_LCD_PrintSpaces(20);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_FX_ROBOTIZE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}
