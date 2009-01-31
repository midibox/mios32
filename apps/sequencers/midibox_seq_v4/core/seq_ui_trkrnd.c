// $Id$
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


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       7
#define ITEM_GXTY          0
#define ITEM_PAR_A         1
#define ITEM_PAR_B         2
#define ITEM_PAR_C         3
#define ITEM_TRG_A         4
#define ITEM_TRG_B         5
#define ITEM_TRG_C         6


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

static u8 random_gen_req;


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 RandomGenerator(u8 req);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_PAR_A: *gp_leds = 0x0100; break;
    case ITEM_PAR_B: *gp_leds = 0x0200; break;
    case ITEM_PAR_C: *gp_leds = 0x0400; break;
    case ITEM_TRG_A: *gp_leds = 0x0800; break;
    case ITEM_TRG_B: *gp_leds = 0x1000; break;
    case ITEM_TRG_C: *gp_leds = 0x2000; break;
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
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      return 0; // no encoder function

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_PAR_A;
      break;
    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_PAR_B;
      break;
    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_PAR_C;
      break;
    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_TRG_A;
      break;
    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_TRG_B;
      break;
    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_TRG_C;
      break;
      
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return 0; // no encoder function
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:   return SEQ_UI_GxTyInc(incrementer);
    case ITEM_PAR_A:  random_gen_req |= 0x01; return SEQ_UI_Var8_Inc(&par_layer_range[0], 0, 63, incrementer);
    case ITEM_PAR_B:  random_gen_req |= 0x02; return SEQ_UI_Var8_Inc(&par_layer_range[1], 0, 63, incrementer);
    case ITEM_PAR_C:  random_gen_req |= 0x04; return SEQ_UI_Var8_Inc(&par_layer_range[2], 0, 63, incrementer);
    case ITEM_TRG_A:  random_gen_req |= 0x08; return SEQ_UI_Var8_Inc(&trg_layer_range[0], 0, 15, incrementer);
    case ITEM_TRG_B:  random_gen_req |= 0x10; return SEQ_UI_Var8_Inc(&trg_layer_range[1], 0, 15, incrementer);
    case ITEM_TRG_C:  random_gen_req |= 0x20; return SEQ_UI_Var8_Inc(&trg_layer_range[2], 0, 15, incrementer);
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
  u8 visible_track = SEQ_UI_VisibleTrackGet();

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
	random_gen_req |= 0x3f;
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

    case SEQ_UI_BUTTON_GP9: // request random generation of certain layer
    case SEQ_UI_BUTTON_GP10:
    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      ui_selected_item = ITEM_PAR_A + (button-8);
      random_gen_req |= (1 << (button-8));
      return 1;

    case SEQ_UI_BUTTON_GP15: // not used
    case SEQ_UI_BUTTON_GP16:
      return 0;

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
  // new requests?
  if( random_gen_req ) {
    portENTER_CRITICAL();
    u8 req = random_gen_req;
    random_gen_req = 0;
    portEXIT_CRITICAL();

    if( req )
      RandomGenerator(req);
  }

  // branch to edit page if random values have been generated
  if( in_menu_msg == MSG_RANDOM )
    return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_RANDOM);

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk.          Random Generator          LayA LayB LayC TrgA TrgB TrgC           
  // G1T1  Generate           Clr. Util Undo  64   --   --   All  --   --            

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.          Random Generator  ");

  if( (in_menu_msg & 0x80) || ((in_menu_msg & 0x7f) && ui_hold_msg_ctr) ) {
    SEQ_LCD_PrintString((char *)in_menu_msg_str[(in_menu_msg & 0x7f)-1]);
  } else {
    SEQ_LCD_PrintSpaces(8);
  }

  SEQ_LCD_PrintString("LayA LayB LayC TrgA TrgB TrgC           ");

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }

  SEQ_LCD_PrintString("  Generate           Clr. Util Undo ");

  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<3; ++i) {
    if( ui_selected_item == (ITEM_PAR_A+i) && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      if( par_layer_range[i] )
	SEQ_LCD_PrintFormattedString(" %2d  ", par_layer_range[i]);
      else
	SEQ_LCD_PrintString(" --  ");
    }
  }

  for(i=0; i<3; ++i) {
    if( ui_selected_item == (ITEM_TRG_A+i) && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      if( trg_layer_range[i] == 15 )
	SEQ_LCD_PrintString(" All ");
      else if( trg_layer_range[i] )
	SEQ_LCD_PrintFormattedString(" %2d  ", trg_layer_range[i]);
      else
	SEQ_LCD_PrintString(" --  ");
    }
  }

  SEQ_LCD_PrintSpaces(10);

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

  random_gen_req = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// The Random Generator
/////////////////////////////////////////////////////////////////////////////
static s32 RandomGenerator(u8 req)
{
  // check for requests
  int i;

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  // TODO: proper parametrisation depending on number of par/trg layers

  // update UNDO buffer
  SEQ_UI_UTIL_UndoUpdate(visible_track);

  for(i=0; i<6; ++i) {
    if( req & (1 << i) )

      ///////////////////////////////////////////////////////////////////////
      // Parameter Layers
      ///////////////////////////////////////////////////////////////////////
      if( i < 3 ) {
	u8 layer = i;

	// don't touch if intensity is 0
	if( !par_layer_range[layer] )
	  continue;

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
	u8 layer = i-3;

	// don't touch if probability is 0
	u8 probability = trg_layer_range[layer];
	if( !probability )
	  continue;

	u16 step;
	u16 num_steps = SEQ_TRG_NumStepsGet(visible_track);
	for(step=0; step<num_steps; ++step) {
	  if( probability == 15 ) // set all steps
	    SEQ_TRG_Set(visible_track, step, layer, ui_selected_instrument, 1);
	  else {
	    u8 rnd = SEQ_RANDOM_Gen_Range(1, 14);
	    SEQ_TRG_Set(visible_track, step, layer, ui_selected_instrument, (probability >= rnd) ? 1 : 0);
	  }
	}

      }
  }

  return 0; // no error
}

