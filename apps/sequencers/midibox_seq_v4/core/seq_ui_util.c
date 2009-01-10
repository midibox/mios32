// $Id$
/*
 * Utility page
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

#include "seq_core.h"
#include "seq_par.h"
#include "seq_trg.h"
#include "seq_cc.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// used "In-Menu" messages
#define MSG_DEFAULT 0x00
#define MSG_COPY    0x81
#define MSG_PASTE   0x82
#define MSG_CLEAR   0x83
#define MSG_MOVE    0x84
#define MSG_SCROLL  0x85
#define MSG_UNDO    0x86


// name the two buffers of the move function
#define MOVE_BUFFER_NEW 0
#define MOVE_BUFFER_OLD 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 in_menu_msg;

static const char in_menu_msg_str[6][9] = {
  ">COPIED<",	// #1
  ">PASTED<",	// #2
  "CLEARED!",	// #3
  ">>MOVE<<",	// #4
  ">SCROLL<",	// #5
  ">>UNDO<<"	// #6
};

static u8 copypaste_begin;
static u8 copypaste_end;

static u8 copypaste_buffer_filled = 0;
static u8 copypaste_par_layer[SEQ_PAR_NUM_LAYERS][SEQ_CORE_NUM_STEPS];
static u8 copypaste_trg_layer[SEQ_TRG_NUM_LAYERS][SEQ_CORE_NUM_STEPS/8];

static u8 undo_buffer_filled = 0;
static u8 undo_track;
static u8 undo_par_layer[SEQ_PAR_NUM_LAYERS][SEQ_CORE_NUM_STEPS];
static u8 undo_trg_layer[SEQ_TRG_NUM_LAYERS][SEQ_CORE_NUM_STEPS/8];

static s8 move_enc;
static u8 move_par_layer[2][SEQ_PAR_NUM_LAYERS];
static u8 move_trg_layer[2];


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 COPY_Track(u8 track);
static s32 PASTE_Track(u8 track);
static s32 CLEAR_Track(u8 track);
static s32 UNDO_Track(void);

static s32 MOVE_StoreStep(u8 track, u8 step, u8 buffer, u8 clr_triggers);
static s32 MOVE_RestoreStep(u8 track, u8 step, u8 buffer);

static s32 SCROLL_Track(u8 track, u8 first_step, s32 incrementer);

static s32 SEQ_UI_UTIL_MuteAllTracks(void);
static s32 SEQ_UI_UTIL_UnMuteAllTracks(void);


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  // branch to edit page if copy/paste/move/scroll button is pressed
  switch( in_menu_msg ) {
    case MSG_COPY:
    case MSG_PASTE:
    case MSG_MOVE:
    case MSG_SCROLL:
      return SEQ_UI_EDIT_LED_Handler(gp_leds);
  }

  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

 *gp_leds = 0x0001;

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
  u8 encoder_step = (u8)encoder + 16*ui_selected_step_view;

  // branch depending on in-menu message
  switch( in_menu_msg ) {
    case MSG_COPY: {
      // change copy offset and copy marker to begin/end range depending on encoder which has been moved
      if( encoder_step <= copypaste_begin ) {
	if( SEQ_UI_Var8_Inc(&copypaste_begin, 0, copypaste_end, incrementer) ) {
	  SEQ_UI_SelectedStepSet(copypaste_begin); // set new visible step/view
	  return 1; // value changed
	}
      } else {
	if( SEQ_UI_Var8_Inc(&copypaste_end, copypaste_begin, SEQ_CC_Get(visible_track, SEQ_CC_LENGTH), incrementer) ) {
	  SEQ_UI_SelectedStepSet(copypaste_end); // set new visible step/view
	  return 1; // value changed
	}
      }
      return 0; // no value changed
    } break;

    case MSG_PASTE: {
      // change paste offset
      if( SEQ_UI_Var8_Inc(&ui_selected_step, 0, SEQ_CORE_NUM_STEPS-1, incrementer) ) {
	SEQ_UI_SelectedStepSet(ui_selected_step); // set new visible step/view
	return 1; // value changed
      }
      return 0; // no value changed
    } break;

    case MSG_MOVE: {
      // if encoder number is different from move_enc, get step, otherwise move step

      if( move_enc != encoder_step ) { // first selection or new encoder is moved

	// select step
	SEQ_UI_SelectedStepSet(encoder_step); // this will change ui_selected_step

	// not first selection -> new encoder
	if( move_enc != -1 ) {
	  // restore moved value
	  MOVE_RestoreStep(visible_track, ui_selected_step, MOVE_BUFFER_NEW);
	}
	// select new encoder
	move_enc = ui_selected_step;
	// store current step value in buffer
	MOVE_StoreStep(visible_track, ui_selected_step, MOVE_BUFFER_NEW, 0);
	// store it also in "old" record and disable current value (clear all triggers)
	MOVE_StoreStep(visible_track, ui_selected_step, MOVE_BUFFER_OLD, 1);

      } else {

	// increment step -> this will move it
	u8 new_step = ui_selected_step;
	if( SEQ_UI_Var8_Inc(&new_step, 0, SEQ_CORE_NUM_STEPS-1, incrementer) ) {
	  // restore old value
	  MOVE_RestoreStep(visible_track, ui_selected_step, MOVE_BUFFER_OLD);
	  // set new visible step/view
	  SEQ_UI_SelectedStepSet(new_step); // this will change ui_selected_step
	  // store "new" old value w/o disabling triggers
	  MOVE_StoreStep(visible_track, new_step, MOVE_BUFFER_OLD, 0);
	  // restore moved value in new step step
	  MOVE_RestoreStep(visible_track, new_step, MOVE_BUFFER_NEW);
	}
      }
      return 1; // value changed
    } break;

    case MSG_SCROLL: {
      // select step
      SEQ_UI_SelectedStepSet(encoder_step); // this will change ui_selected_step
      // call scroll handler
      SCROLL_Track(visible_track, ui_selected_step, incrementer);
      return 1; // value changed
    } break;

    default:
      if( encoder == SEQ_UI_ENCODER_GP1 )
	return SEQ_UI_GxTyInc(incrementer);
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
      return 1; // nothing to do for button

    case SEQ_UI_BUTTON_GP2: // Copy
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
	// copy steps
	COPY_Track(visible_track);
      } else {
	// print message
	in_menu_msg = MSG_COPY;
	// select first step
	SEQ_UI_SelectedStepSet(0);
	// select full range
	copypaste_begin = 0;
	copypaste_end = SEQ_CC_Get(visible_track, SEQ_CC_LENGTH);
      }
      return 1;

    case SEQ_UI_BUTTON_GP3: // Paste
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
	// paste steps
	PASTE_Track(visible_track);
      } else {
	// update undo buffer
	SEQ_UI_UTIL_UndoUpdate(visible_track);
	// print message
	in_menu_msg = MSG_PASTE;
	// select first step
	SEQ_UI_SelectedStepSet(0);
      }
      return 1;

    case SEQ_UI_BUTTON_GP4: // Clear
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	// update undo buffer
	SEQ_UI_UTIL_UndoUpdate(visible_track);
	// clear steps
	CLEAR_Track(visible_track);
	// print message
	in_menu_msg = MSG_CLEAR;
      }
      return 1;

    case SEQ_UI_BUTTON_GP5: // Move
      move_enc = -1; // disable move encoder
      if( depressed ) {
	// turn message inactive and hold it for 0.5 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 500;
      } else {
	// update undo buffer
	SEQ_UI_UTIL_UndoUpdate(visible_track);
	// print message
	in_menu_msg = MSG_MOVE;
      }
      return 1;

    case SEQ_UI_BUTTON_GP6: // Scroll
      if( depressed ) {
	// turn message inactive and hold it for 0.5 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 500;
      } else {
	// update undo buffer
	SEQ_UI_UTIL_UndoUpdate(visible_track);
	// print message
	in_menu_msg = MSG_SCROLL;
	// select first step
	SEQ_UI_SelectedStepSet(0);
      }
      return 1;

    case SEQ_UI_BUTTON_GP7: // select Random Page
      if( depressed ) return -1;
      SEQ_UI_PageSet(SEQ_UI_PAGE_TRKRND);
      return 0;
      
    case SEQ_UI_BUTTON_GP8: // Undo
      if( depressed ) {
	// turn message inactive and hold it for 1 second
	in_menu_msg &= 0x7f;
	ui_hold_msg_ctr = 1000;
      } else {
	// undo last change
	UNDO_Track();
	// print message
	in_menu_msg = MSG_UNDO;
      }
      return 1;

    case SEQ_UI_BUTTON_GP9: // select Save Page
      if( depressed ) return -1;
      SEQ_UI_PageSet(SEQ_UI_PAGE_SAVE);
      return 0;

    case SEQ_UI_BUTTON_GP10: // select Record Page
      if( depressed ) return -1;
      return -1; // TODO

    case SEQ_UI_BUTTON_GP11: // select Mixer Page
      if( depressed ) return -1;
      return -1; // TODO

    case SEQ_UI_BUTTON_GP12: // select Options Page
      if( depressed ) return -1;
      SEQ_UI_PageSet(SEQ_UI_PAGE_OPT);
      return 0;

    case SEQ_UI_BUTTON_GP13: // select Port Mute page
      if( depressed ) return -1;
      return -1; // TODO

    case SEQ_UI_BUTTON_GP14: // free
      if( depressed ) return -1;
      return -1; // TODO

    case SEQ_UI_BUTTON_GP15: // mute all tracks
      if( depressed ) return -1;
      SEQ_UI_UTIL_MuteAllTracks();
      SEQ_UI_PageSet(SEQ_UI_PAGE_MUTE);
      return 1;

    case SEQ_UI_BUTTON_GP16: // unmute all tracks
      if( depressed ) return -1;
      SEQ_UI_UTIL_UnMuteAllTracks();
      SEQ_UI_PageSet(SEQ_UI_PAGE_MUTE);
      return 1;

    case SEQ_UI_BUTTON_Select:
    case SEQ_UI_BUTTON_Right:
      if( depressed ) return -1;
      return 0;

    case SEQ_UI_BUTTON_Left:
      if( depressed ) return -1;
      return 0;

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
  // branch to edit page if copy/paste/move/scroll button is pressed
  switch( in_menu_msg ) {
    case MSG_COPY: return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_COPY);
    case MSG_PASTE: return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_PASTE);
    case MSG_MOVE: return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_MOVE);
    case MSG_SCROLL: return SEQ_UI_EDIT_LCD_Handler(high_prio, SEQ_UI_EDIT_MODE_SCROLL);
  }

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk.        Utility Functions                       Quick Menu Change           
  // G1T1 Copy Paste Clr Move Scrl Rand Undo Save Rec. Mix. Opt. PMte     Mute UnMute

  u8 visible_track = SEQ_UI_VisibleTrackGet();

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk.        Utility Functions   ");

  if( (in_menu_msg & 0x80) || ((in_menu_msg & 0x7f) && ui_hold_msg_ctr) ) {
    SEQ_LCD_PrintString((char *)in_menu_msg_str[(in_menu_msg & 0x7f)-1]);
  } else {
    SEQ_LCD_PrintSpaces(8);
  }
  SEQ_LCD_PrintString("            Quick Menu Change           ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  //  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
  if( ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
  }

  SEQ_LCD_PrintString(" Copy Paste Clr Move Scrl Rand Undo ");
  SEQ_LCD_PrintString("Save Rec. Mix. Opt. PMte     Mute UnMute");

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  ui_hold_msg_ctr = 0;

  //  copypaste_begin = copypaste_end = 0;
  // in_menu_msg = MSG_DEFAULT; // clashes with external Copy/Paste button function, since
  // SEQ_UI_UTIL_Init() function will be called later by background task, while buttons are handled
  // by SRIO task -- therefore don't init in_menu_msg and copypaste pointers

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Functions used by SEQ_UI_EDIT
/////////////////////////////////////////////////////////////////////////////
u8 SEQ_UI_UTIL_CopyPasteBeginGet(void)
{
  return copypaste_begin;
}

u8 SEQ_UI_UTIL_CopyPasteEndGet(void)
{
  return copypaste_end;
}


/////////////////////////////////////////////////////////////////////////////
// Copy track
/////////////////////////////////////////////////////////////////////////////
static s32 COPY_Track(u8 track)
{
  u8 layer;
  u8 step;

  // copy layers into buffer
  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer)
    for(step=0; step<SEQ_CORE_NUM_STEPS; ++step)
      copypaste_par_layer[layer][step] = SEQ_PAR_Get(track, step, layer);

  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
    for(step=0; step<(SEQ_CORE_NUM_STEPS/8); ++step)
      copypaste_trg_layer[layer][step] = SEQ_TRG_Get8(track, step, layer);

  // notify that copy&paste buffer is filled
  copypaste_buffer_filled = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Paste a track with selectable offset (stored in ui_selected_step)
/////////////////////////////////////////////////////////////////////////////
static s32 PASTE_Track(u8 track)
{
  u8 layer;
  u8 step;

  // branch to clear function if copy&paste buffer not filled
  if( !copypaste_buffer_filled )
    return CLEAR_Track(track);

  // determine begin/end boundary
  u8 step_begin = copypaste_begin;
  u8 step_end = copypaste_end;
  // swap if required
  if( step_begin > step_end ) {
    u8 tmp = step_end;
    step_end = step_begin;
    step_begin = tmp;
  }

  // copy layers from buffer
  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer) {
    u8 step_offset = ui_selected_step;
    for(step=step_begin; step<=step_end; ++step, ++step_offset) {
      if( step_offset < SEQ_CORE_NUM_STEPS )
	SEQ_PAR_Set(track, step_offset, layer, copypaste_par_layer[layer][step]);
    }
  }

  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer) {
    u8 step_offset = ui_selected_step;
    for(step=step_begin; step<=step_end; ++step, ++step_offset) {
      if( step_offset < SEQ_CORE_NUM_STEPS ) {
	u8 step_ix = step>>3;
	u8 step_mask = (1 << (step&7));
	SEQ_TRG_Set(track, step_offset, layer, (copypaste_trg_layer[layer][step_ix] & step_mask) ? 1 : 0);
      }
    }
  }

  if( seq_core_options.PASTE_CLR_ALL ) {
    // TODO: copy track configuration
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// clear a track
/////////////////////////////////////////////////////////////////////////////
static s32 CLEAR_Track(u8 track)
{
  // copy preset
  SEQ_LAYER_CopyPreset(track, seq_core_options.PASTE_CLR_ALL ? 0 : 1);

  // clear all triggers
  u8 layer;
  u8 step;
  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
    for(step=0; step<(SEQ_CORE_NUM_STEPS/8); ++step)
      SEQ_TRG_Set8(track, step, layer, 0x00);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// UnDo function
/////////////////////////////////////////////////////////////////////////////
static s32 UNDO_Track(void)
{
  u8 layer;
  u8 step;

  // exit if undo buffer not filled
  if( !undo_buffer_filled )
    return;

  // copy layers from buffer
  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer)
    for(step=0; step<SEQ_CORE_NUM_STEPS; ++step)
      SEQ_PAR_Set(undo_track, step, layer, undo_par_layer[layer][step]);

  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
    for(step=0; step<(SEQ_CORE_NUM_STEPS/8); ++step)
      SEQ_TRG_Set8(undo_track, step, layer, undo_trg_layer[layer][step]);

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Updates the UnDo buffer - can also be called from external (e.g. TRKRND)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_UndoUpdate(u8 track)
{
  u8 layer;
  u8 step;

  // store track in special variable, so that we restore to the right one later
  undo_track = track;

  // copy layers into buffer
  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer)
    for(step=0; step<SEQ_CORE_NUM_STEPS; ++step)
      undo_par_layer[layer][step] = SEQ_PAR_Get(undo_track, step, layer);

  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
    for(step=0; step<(SEQ_CORE_NUM_STEPS/8); ++step)
      undo_trg_layer[layer][step] = SEQ_TRG_Get8(undo_track, step, layer);

  // notify that copy&paste buffer is filled
  undo_buffer_filled = 1;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help functions for move step feature
/////////////////////////////////////////////////////////////////////////////
static s32 MOVE_StoreStep(u8 track, u8 step, u8 buffer, u8 clr_triggers)
{
  u8 layer;

  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer)
    move_par_layer[buffer][layer] = SEQ_PAR_Get(track, step, layer);

  move_trg_layer[buffer] = 0;
  if( !clr_triggers ) {
    for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
      if( SEQ_TRG_Get(track, step, layer) )
	move_trg_layer[buffer] |= (1 << layer);
  }

  return 0; // no error
}

static s32 MOVE_RestoreStep(u8 track, u8 step, u8 buffer)
{
  u8 layer;

  for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer)
    SEQ_PAR_Set(track, step, layer, move_par_layer[buffer][layer]);

  for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer)
    SEQ_TRG_Set(track, step, layer, (move_trg_layer[buffer] & (1 << layer)) ? 1 : 0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Scroll function
/////////////////////////////////////////////////////////////////////////////
static s32 SCROLL_Track(u8 track, u8 first_step, s32 incrementer)
{
  u8 layer;
  u8 step;

  // determine the last step which has to be rotated
  u8 last_step = SEQ_CC_Get(track, SEQ_CC_LENGTH);
  if( first_step > last_step ) {
    // loop point behind track len -> rotate complete track
    first_step = 0;
    last_step = SEQ_CORE_NUM_STEPS-1;
  }

  if( first_step < last_step ) {
    if( incrementer >= 0 ) {
      // rightrotate parameter layers
      for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer) {
	u8 tmp = SEQ_PAR_Get(track, last_step, layer);
	for(step=last_step; step>first_step; --step)
	  SEQ_PAR_Set(track, step, layer, SEQ_PAR_Get(track, step-1, layer));
	SEQ_PAR_Set(track, step, layer, tmp);
      }

      // rightrotate trigger layers
      for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer) {
	u8 tmp = SEQ_TRG_Get(track, last_step, layer);
	for(step=last_step; step>first_step; --step)
	  SEQ_TRG_Set(track, step, layer, SEQ_TRG_Get(track, step-1, layer));
	SEQ_TRG_Set(track, step, layer, tmp);
      }
    } else {
      // leftrotate parameter layers
      for(layer=0; layer<SEQ_PAR_NUM_LAYERS; ++layer) {
	u8 tmp = SEQ_PAR_Get(track, first_step, layer);
	for(step=first_step; step<last_step; ++step)
	  SEQ_PAR_Set(track, step, layer, SEQ_PAR_Get(track, step+1, layer));
	SEQ_PAR_Set(track, step, layer, tmp);
      }

      // leftrotate triggerr layers
      for(layer=0; layer<SEQ_TRG_NUM_LAYERS; ++layer) {
	u8 tmp = SEQ_TRG_Get(track, first_step, layer);
	for(step=first_step; step<last_step; ++step)
	  SEQ_TRG_Set(track, step, layer, SEQ_TRG_Get(track, step+1, layer));
	SEQ_TRG_Set(track, step, layer, tmp);
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// This function mutes all tracks
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_UTIL_MuteAllTracks(void)
{
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    seq_core_trk[track].state.MUTED = 1;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// This function unmutes all tracks
/////////////////////////////////////////////////////////////////////////////
static s32 SEQ_UI_UTIL_UnMuteAllTracks(void)
{
  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    seq_core_trk[track].state.MUTED = 0;

  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
// For direct access to copy/paste/clear/undo function
// (e.g. via special button, but also from other pages -> TRKRND)
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_UTIL_CopyButton(s32 depressed)
{
  return Button_Handler(SEQ_UI_BUTTON_GP2, depressed);
}

s32 SEQ_UI_UTIL_PasteButton(s32 depressed)
{
  return Button_Handler(SEQ_UI_BUTTON_GP3, depressed);
}

s32 SEQ_UI_UTIL_ClearButton(s32 depressed)
{
  return Button_Handler(SEQ_UI_BUTTON_GP4, depressed);
}

s32 SEQ_UI_UTIL_UndoButton(s32 depressed)
{
  return Button_Handler(SEQ_UI_BUTTON_GP8, depressed);
}

