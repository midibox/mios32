// $Id: seq_ui_trkrnd.c 1483 2012-07-01 17:54:40Z tk $
/*
 * Random Generator page
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

#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_layer.h"
#include "seq_cc.h"
#include "seq_random.h"
#include <glcd_font.h>

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       7
#define ITEM_GXTY          0
#define ITEM_SCROLL        1
#define ITEM_PAR1          2
#define ITEM_PAR2          3
#define ITEM_PAR3          4
#define ITEM_PAR4          5
#define ITEM_PAR5          6
#define ITEM_PAR6          7
#define ITEM_PAR7          8


// used "In-Menu" messages
#define MSG_DEFAULT 0x00
#define MSG_RANDOM  0x81
#define MSG_CLEAR   0x82
#define MSG_UNDO    0x83


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 in_menu_msg;

static const char in_menu_msg_str[3][9] = {
  ">RANDOM<",	// #1
  "CLEARED!",	// #2
  ">>UNDO<<"	// #3
};


// 0..63
static u8 par_layer_range[16] = { 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
// 0..14, 15=ALL
static u8 trg_layer_range[16] = { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static u8 scroll_offset = 0;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 RandomGenerator(u32 req);
static int getRandomIx(int rnd_item);

/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY:   *gp_leds = 0x0001; break;
    case ITEM_SCROLL: *gp_leds = 0x0100; break;
    case ITEM_PAR1:   *gp_leds = 0x0200; break;
    case ITEM_PAR2:   *gp_leds = 0x0400; break;
    case ITEM_PAR3:   *gp_leds = 0x0800; break;
    case ITEM_PAR4:   *gp_leds = 0x1000; break;
    case ITEM_PAR5:   *gp_leds = 0x2000; break;
    case ITEM_PAR6:   *gp_leds = 0x4000; break;
    case ITEM_PAR7:   *gp_leds = 0x8000; break;
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
    //  ui_selected_item = ITEM_GXTY;
	if( SEQ_UI_Var8_Inc(&ui_selected_item, 0, NUM_OF_ITEMS-1, incrementer) >= 0 )
		return 1;
	else
		return 0; 	  
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      return 0; // no encoder function

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_SCROLL;
      break;
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_PAR1;
      break;
    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_PAR2;
      break;
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_PAR3;
      break;
    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_PAR4;
      break;
    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_PAR5;
      break;
    case SEQ_UI_ENCODER_GP15:
      ui_selected_item = ITEM_PAR6;
      break;
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_PAR7;
      break;
  }

  // for GP encoders and Datawheel
  int rnd_item = -1;
 if (encoder == SEQ_UI_ENCODER_Datawheel) {    
  switch( ui_selected_item ) {
  case ITEM_GXTY:   return SEQ_UI_GxTyInc(incrementer);
  case ITEM_SCROLL: {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
    int num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
    int num_t_layers = (event_mode == SEQ_EVENT_MODE_Drum) ? SEQ_TRG_NumInstrumentsGet(visible_track) : SEQ_TRG_NumLayersGet(visible_track);
    int max_offset = num_p_layers + num_t_layers - 7;
    if( max_offset < 1 ) {
      scroll_offset = 0;
      return -1;
    }
    return SEQ_UI_Var8_Inc(&scroll_offset, 0, max_offset, incrementer);
  }
  case ITEM_PAR1:   rnd_item = 0; break;
  case ITEM_PAR2:   rnd_item = 1; break;
  case ITEM_PAR3:   rnd_item = 2; break;
  case ITEM_PAR4:   rnd_item = 3; break;
  case ITEM_PAR5:   rnd_item = 4; break;
  case ITEM_PAR6:   rnd_item = 5; break;
  case ITEM_PAR7:   rnd_item = 6; break;
  }
}
  if( rnd_item >= 0 ) {
    int rnd_ix = getRandomIx(rnd_item);
    if( rnd_ix < 0 )
      return -1;

    s32 status = -1;
    if( rnd_ix < 16 )
      status = SEQ_UI_Var8_Inc(&par_layer_range[rnd_ix], 0, 63, incrementer);
    else if( rnd_item < 32 )
      status = SEQ_UI_Var8_Inc(&trg_layer_range[rnd_ix-16], 0, 15, incrementer);

    return status;
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
  switch( button ) {
    case SEQ_UI_BUTTON_GP1: // GxTy
      if( depressed ) return 0; // ignore when button depressed
      ui_selected_item = ITEM_GXTY;
      return 1;

    case SEQ_UI_BUTTON_GP2: // Generate
    case SEQ_UI_BUTTON_GP3:
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	// request new values for all layers
	RandomGenerator(0xffffffff);

	// print message
	in_menu_msg = MSG_RANDOM;
      }
      return 1;

    case SEQ_UI_BUTTON_GP4: // not used
    case SEQ_UI_BUTTON_GP5:
      return 0;

    case SEQ_UI_BUTTON_GP6: // Clear
      SEQ_UI_UTIL_ClearButton(depressed);
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	// print message
	in_menu_msg = MSG_CLEAR;
      }
      return 1;

    case SEQ_UI_BUTTON_GP7: // back to util page
      if( depressed ) return -1;
      SEQ_UI_PageSet(SEQ_UI_PAGE_UTIL);
      return 0;

    case SEQ_UI_BUTTON_GP8: // Undo
      SEQ_UI_UTIL_UndoButton(depressed);
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	// print message
	in_menu_msg = MSG_UNDO;
      }
      return 1;

    case SEQ_UI_BUTTON_GP9:
      if( depressed ) return -1;
      ui_selected_item = ITEM_SCROLL;
      return 1;

    case SEQ_UI_BUTTON_GP10: // request random generation of certain layer
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	ui_selected_item = ITEM_PAR1 + (button-9);

	int rnd_ix = getRandomIx(button - 9);
	if( rnd_ix < 0 )
	  return -1;

	// request new values for selected layer
	RandomGenerator(1 << rnd_ix);

	// print message
	in_menu_msg = MSG_RANDOM;
      }
      return 1;

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
  // branch to edit page if random values have been generated
  if( in_menu_msg == MSG_RANDOM )
    return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_RANDOM);

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk.          Random Generator          Scrl LayA LayB LayC LayD LayE LayF LayG 
  // G1T1  Generate           Clr. Util Undo  <>   64   --   --   All  --   --   --

  ///////////////////////////////////////////////////////////////////////////
  //SEQ_LCD_CursorSet(0, 0);
  //SEQ_LCD_PrintString("Trk.          Random Generator  ");
  
  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  if( ui_selected_item == ITEM_GXTY ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    //SEQ_LCD_PrintSpaces(2);
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
	SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    //SEQ_LCD_PrintSpaces(2);
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } 
  SEQ_LCD_PrintString(" TRCK RANDOM GEN ");
  
  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  if( (in_menu_msg & 0x80) || ((in_menu_msg & 0x7f) && ui_hold_msg_ctr) ) {
    SEQ_LCD_PrintString((char *)in_menu_msg_str[(in_menu_msg & 0x7f)-1]);
  } else {
    SEQ_LCD_PrintSpaces(21);
  }
  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 2);
  if( ui_selected_item == ITEM_SCROLL ) {
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL_INV);
    SEQ_LCD_PrintString("Scrl ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } else {
    SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
    SEQ_LCD_PrintString("Scrl ");
	SEQ_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  } 


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 3);
//  if( ui_selected_item == ITEM_SCROLL && ui_cursor_flash ) {
//    SEQ_LCD_PrintSpaces(5);
//  } else {
    SEQ_LCD_PrintString(" <>  ");
// }  
  
  {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

    int i;
    for(i=0; i<7; ++i) {
      int rnd_ix = getRandomIx(i);
      if( rnd_ix < 0 ) {
	// dummy...
	SEQ_LCD_CursorSet(5 + i*5, 2);
	SEQ_LCD_PrintString("???? ");
  ///////////////////////////////////////////////////////////////////////////	
	SEQ_LCD_CursorSet(5 + i*5, 3);
	SEQ_LCD_PrintString("???? ");
      } else {
	if( rnd_ix < 16 ) {
  ///////////////////////////////////////////////////////////////////////////		
	  SEQ_LCD_CursorSet(5 + i*5, 2);
	  SEQ_LCD_PrintFormattedString("Lay%c ", 'A' + rnd_ix);

  ///////////////////////////////////////////////////////////////////////////		  
	  SEQ_LCD_CursorSet(5 + i*5, 3);
	  if( ui_selected_item == (ITEM_PAR1+i) && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(5);
	  } else {
	    if( par_layer_range[rnd_ix] )
	      SEQ_LCD_PrintFormattedString(" %2d  ", par_layer_range[rnd_ix]);
	    else
	      SEQ_LCD_PrintString(" --  ");
	  }
	} else if( rnd_ix < 32 ) {
	  int trg_ix = rnd_ix-16;
  ///////////////////////////////////////////////////////////////////////////	
	  //SEQ_LCD_CursorSet(5 + i*5, 4);
	  SEQ_LCD_CursorSet(trg_ix*5, 4);
	  if( event_mode == SEQ_EVENT_MODE_Drum )
	    SEQ_LCD_PrintTrackDrum(visible_track, trg_ix, (char *)seq_core_trk[visible_track].name);
	  else
	    SEQ_LCD_PrintFormattedString("Trg%c ", 'A' + trg_ix);
  ///////////////////////////////////////////////////////////////////////////	
	  //SEQ_LCD_CursorSet(5 + i*5, 5);
	  SEQ_LCD_CursorSet(trg_ix*5, 5);
	  if( ui_selected_item == (ITEM_PAR1+i) && ui_cursor_flash ) {
	    SEQ_LCD_PrintSpaces(5);
	  } else {
	    if( trg_layer_range[trg_ix] == 15 )
	      SEQ_LCD_PrintString(" All ");
	    else if( trg_layer_range[trg_ix] )
	      SEQ_LCD_PrintFormattedString(" %2d  ", trg_layer_range[trg_ix]);
	    else
	      SEQ_LCD_PrintString(" --  ");
	  }
	}
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 7);
/*
  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }
*/
  SEQ_LCD_PrintString("Gener. Clr. Util Undo");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKRND_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Returns random request flag (rnd_ix) depending on scroll offset
// 0..15: parameter layers
// 16..31: trigger/instrument layers
/////////////////////////////////////////////////////////////////////////////
static int getRandomIx(int rnd_item)
{
  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
  int num_p_layers = SEQ_PAR_NumLayersGet(visible_track);
  int num_t_layers = (event_mode == SEQ_EVENT_MODE_Drum) ? SEQ_TRG_NumInstrumentsGet(visible_track) : SEQ_TRG_NumLayersGet(visible_track);

  rnd_item += scroll_offset;
  if( rnd_item >= (num_p_layers+num_t_layers) ) {
    scroll_offset = 0;
    // try again...
    return -1;
  }

  if( rnd_item < num_p_layers )
    return rnd_item;

  return 16 + (rnd_item-num_p_layers);
}



/////////////////////////////////////////////////////////////////////////////
// The Random Generator
/////////////////////////////////////////////////////////////////////////////
static s32 RandomGenerator(u32 req)
{
  // check for requests
  int i;

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // update UNDO buffer
  SEQ_UI_UTIL_UndoUpdate(visible_track);

  for(i=0; i<32; ++i) {
    if( req & (1 << i) ) {

      ///////////////////////////////////////////////////////////////////////
      // Parameter Layers
      ///////////////////////////////////////////////////////////////////////
      if( i < 16 ) {
	u8 layer = i;

	// don't touch if intensity is 0
	if( !par_layer_range[layer] )
	  continue;

	// select parameter layer
	ui_selected_par_layer = layer;

	// determine range
	u8 base, range;
	seq_par_layer_type_t layer_type = SEQ_PAR_AssignmentGet(visible_track, layer);
	if( layer_type == SEQ_PAR_Type_Length ) {
	  base = 16;
	  range = ((par_layer_range[layer] >> 2) - 1); // -> +/- 16
	  if( !range || range > 128 ) // (on underrun...)
	    range = 1;
	} else {
	  base = 0x40;
	  range = par_layer_range[layer];
	}

	// set random values
	u16 step;
	u16 num_steps = SEQ_PAR_NumStepsGet(visible_track);
	for(step=0; step<num_steps; ++step)
	  SEQ_PAR_Set(visible_track, step, layer, ui_selected_instrument, SEQ_RANDOM_Gen_Range(base-range, base+range));


      ///////////////////////////////////////////////////////////////////////
      // Trigger Layers
      ///////////////////////////////////////////////////////////////////////
      } else {
	u8 layer = i-16;

	if( event_mode == SEQ_EVENT_MODE_Drum ) {
	  layer = 0;
	  ui_selected_instrument = i - 16;
	} else {
	  ui_selected_trg_layer = layer;
	}

	u8 instrument = ui_selected_instrument;

	// don't touch if probability is 0
	u8 probability = trg_layer_range[i-16];
	if( !probability )
	  continue;

	// select trigger layer
	u16 step;
	u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
	for(step=0; step<num_steps; ++step) {
	  if( probability >= 15 ) // set all steps
	    SEQ_TRG_Set(visible_track, step, layer, instrument, 1);
	  else {
	    u8 rnd = SEQ_RANDOM_Gen_Range(1, 14);
	    SEQ_TRG_Set(visible_track, step, layer, instrument, (probability >= rnd) ? 1 : 0);
	  }
	}

      }
    }
  }

  return 0; // no error
}
