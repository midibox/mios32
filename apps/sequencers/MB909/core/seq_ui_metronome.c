// $Id: seq_ui_metronome.c 1142 2011-02-17 23:19:36Z tk $
/*
 * Metronome configuration page
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

#include "seq_file_gc.h"

#include "seq_midi_port.h"
#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS           5
#define ITEM_ENABLE            0
#define ITEM_PORT              1
#define ITEM_CHANNEL           2
#define ITEM_NOTE_M            3
#define ITEM_NOTE_B            4


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

u8 store_file_required;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_ENABLE: *gp_leds |= 0x0003; break;
    case ITEM_PORT: *gp_leds |= 0x0004; break;
    case ITEM_CHANNEL: *gp_leds |= 0x0008; break;
    case ITEM_NOTE_M: *gp_leds |= 0x0030; break;
    case ITEM_NOTE_B: *gp_leds |= 0x00c0; break;
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
    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_ENABLE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_PORT;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_CHANNEL;
      break;

    case SEQ_UI_ENCODER_GP5:
    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_NOTE_M;
      break;

    case SEQ_UI_ENCODER_GP7:
    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_NOTE_B;
      break;

    case SEQ_UI_ENCODER_GP9:
    case SEQ_UI_ENCODER_GP10:
    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      return -1; // not used (yet)
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_ENABLE:
      if( incrementer )
	seq_core_state.METRONOME = (incrementer > 0) ? 1 : 0;
      return 1;

    case ITEM_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(seq_core_metronome_port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) >= 0 ) {
	seq_core_metronome_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_CHANNEL:
      if( SEQ_UI_Var8_Inc(&seq_core_metronome_chn, 0, 16, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_NOTE_M:
      if( SEQ_UI_Var8_Inc(&seq_core_metronome_note_m, 0, 127, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_NOTE_B:
      if( SEQ_UI_Var8_Inc(&seq_core_metronome_note_b, 0, 127, incrementer) >= 0 ) {
	store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
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

  // GP button selects preset
  if( button <= SEQ_UI_BUTTON_GP16 ) {
    // GP1/2 toggles metronome function
    if( button == SEQ_UI_BUTTON_GP1 || button == SEQ_UI_BUTTON_GP2 )
      seq_core_state.METRONOME ^= 1;

    // re-use encoder function
    return Encoder_Handler(button, 0);
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
  // Metronome Port Chn. Meas.Note  BeatNote 
  //    off    Def. #10     C#1       C#1    


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);

  SEQ_LCD_PrintString("Metronome Port Chn. Meas.Note  BeatNote ");
  SEQ_LCD_PrintSpaces(40);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  SEQ_LCD_PrintSpaces(3);
  if( ui_selected_item == ITEM_ENABLE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintString(seq_core_state.METRONOME ? "on " : "off");
  }
  SEQ_LCD_PrintSpaces(4);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutNameGet(SEQ_MIDI_PORT_OutIxGet(seq_core_metronome_port)));
  }
  SEQ_LCD_PrintSpaces(1);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_CHANNEL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( !seq_core_metronome_chn )
      SEQ_LCD_PrintString("---");
    else
      SEQ_LCD_PrintFormattedString("#%2d", seq_core_metronome_chn);
  }
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_NOTE_M && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(seq_core_metronome_note_m);
  }
  SEQ_LCD_PrintSpaces(7);

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_NOTE_B && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(seq_core_metronome_note_b);
  }
  SEQ_LCD_PrintSpaces(4 + 40);


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Local exit function
/////////////////////////////////////////////////////////////////////////////
static s32 EXIT_Handler(void)
{
  s32 status = 0;

  if( store_file_required ) {
    // write config file
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_GC_Write()) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_METRONOME_Init(u32 mode)
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
