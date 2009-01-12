// $Id$
/*
 * Track clock divider page
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
#include "seq_layer.h"


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       8
#define ITEM_GXTY          0
#define ITEM_EVNT_MODE     1
#define ITEM_EVNT_CONST1   2
#define ITEM_EVNT_CONST2   3
#define ITEM_EVNT_CONST3   4
#define ITEM_MIDI_CHANNEL  5
#define ITEM_MIDI_PORT     6
#define ITEM_PRESET        7


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static s32 PrintCValue(u8 track, u8 const_ix);
static s32 CopyPreset(u8 track);


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 preset_copied;
static u8 old_evnt_mode[SEQ_CORE_NUM_TRACKS];


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  if( ui_cursor_flash ) { // if flashing flag active: no LED flag set
    // flash preset LEDs if current preset doesn't match with old preset
    // this notifies the user, that he should press the "copy preset" button
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    if( old_evnt_mode[visible_track] != SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_MODE) )
      *gp_leds = 0xc000;

    return 0;
  }

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_EVNT_MODE: *gp_leds = 0x001e; break;
    case ITEM_EVNT_CONST1: *gp_leds = 0x0020; break;
    case ITEM_EVNT_CONST2: *gp_leds = 0x0040; break;
    case ITEM_EVNT_CONST3: *gp_leds = 0x0080; break;
    case ITEM_MIDI_CHANNEL: *gp_leds = 0x0100; break;
    case ITEM_MIDI_PORT: *gp_leds = 0x0200; break;
    // case ITEM_PRESET: *gp_leds = 0xc000; break;
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
  preset_copied = 0; // for display notification

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
    case SEQ_UI_ENCODER_GP3:
    case SEQ_UI_ENCODER_GP4:
    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_EVNT_MODE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_EVNT_CONST1;
      break;

    case SEQ_UI_ENCODER_GP7:
      ui_selected_item = ITEM_EVNT_CONST2;
      break;

    case SEQ_UI_ENCODER_GP8:
      ui_selected_item = ITEM_EVNT_CONST3;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_MIDI_PORT;
      break;

    case SEQ_UI_ENCODER_GP11:
    case SEQ_UI_ENCODER_GP12:
    case SEQ_UI_ENCODER_GP13:
    case SEQ_UI_ENCODER_GP14:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP15:
    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_PRESET;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY:          return SEQ_UI_GxTyInc(incrementer);
    case ITEM_EVNT_MODE:     return SEQ_UI_CC_Inc(SEQ_CC_MIDI_EVNT_MODE, 0, SEQ_LAYER_EVNTMODE_NUM-1, incrementer);
    case ITEM_EVNT_CONST1:   return SEQ_UI_CC_Inc(SEQ_CC_MIDI_EVNT_CONST1, 0, 127, incrementer);
    case ITEM_EVNT_CONST2:   return SEQ_UI_CC_Inc(SEQ_CC_MIDI_EVNT_CONST2, 0, 127, incrementer);
    case ITEM_EVNT_CONST3:   return SEQ_UI_CC_Inc(SEQ_CC_MIDI_EVNT_CONST3, 0, 127, incrementer);
    case ITEM_MIDI_CHANNEL:  return SEQ_UI_CC_Inc(SEQ_CC_MIDI_CHANNEL, 0, 15, incrementer);
    case ITEM_MIDI_PORT: {
      u8 visible_track = SEQ_UI_VisibleTrackGet();
      mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
      u8 port_ix = SEQ_MIDI_PORT_OutIxGet(port);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_OutNumGet()-1, incrementer) ) {
	mios32_midi_port_t new_port = SEQ_MIDI_PORT_OutPortGet(port_ix);
	SEQ_UI_CC_Set(SEQ_CC_MIDI_PORT, new_port);
	return 1; // value changed
      }
      return 0; // value not changed
    } break;
    case ITEM_PRESET:        CopyPreset(SEQ_UI_VisibleTrackGet()); return 1;
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
  preset_copied = 0; // for display notification - always clear this flag when any button has been pressed/depressed

  if( depressed ) return 0; // ignore when button depressed

  switch( button ) {
    case SEQ_UI_BUTTON_GP1:
      ui_selected_item = ITEM_GXTY;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP2:
    case SEQ_UI_BUTTON_GP3:
    case SEQ_UI_BUTTON_GP4:
    case SEQ_UI_BUTTON_GP5:
      ui_selected_item = ITEM_EVNT_MODE;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP6:
      ui_selected_item = ITEM_EVNT_CONST1;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP7:
      ui_selected_item = ITEM_EVNT_CONST2;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP8:
      ui_selected_item = ITEM_EVNT_CONST3;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP9:
      ui_selected_item = ITEM_MIDI_CHANNEL;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP10:
      ui_selected_item = ITEM_MIDI_PORT;
      return 0; // value hasn't been changed

    case SEQ_UI_BUTTON_GP11:
    case SEQ_UI_BUTTON_GP12:
    case SEQ_UI_BUTTON_GP13:
    case SEQ_UI_BUTTON_GP14:
      return -1; // not mapped

    case SEQ_UI_BUTTON_GP15:
    case SEQ_UI_BUTTON_GP16:
      // ui_selected_item = ITEM_PRESET;
      CopyPreset(SEQ_UI_VisibleTrackGet());
      return 1; // value has been changed

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
  // Trk. Layer Assignments:xx Val1 Val2 Val3Chn. Port                     >> COPY <<
  // G1T1 A:Chrd B:Vel. C:Len.  127  127  127 16  IIC2 (not available)     > PRESET <

  u8 visible_track = SEQ_UI_VisibleTrackGet();
  u8 evnt_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_MODE);

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  SEQ_LCD_PrintString("Trk. Layer Assignments:");
  ///
  if( ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(2);
  } else {
    SEQ_LCD_PrintFormattedString("%2d", evnt_mode + 1);
  }
  SEQ_LCD_PrintFormattedString(" %s %s %s", 
				  SEQ_LAYER_CTypeStr(evnt_mode, 0),
				  SEQ_LAYER_CTypeStr(evnt_mode, 1),
				  SEQ_LAYER_CTypeStr(evnt_mode, 2));
  ///
  SEQ_LCD_PrintString("Chn. Port                     "); // ">> COPY <<" will be printed later


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);

  if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintSpaces(1);
  }

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "A:");
  SEQ_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 0));
  SEQ_LCD_PrintSpaces(1);

  SEQ_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "B:");
  SEQ_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 1));
  SEQ_LCD_PrintSpaces(1);

  SEQ_LCD_PrintString((ui_selected_item == ITEM_EVNT_MODE && ui_cursor_flash) ? "  " : "C:");
  SEQ_LCD_PrintString(SEQ_LAYER_VTypeStr(evnt_mode, 2));


  ///////////////////////////////////////////////////////////////////////////
  int i;
  for(i=0; i<3; ++i) {
    if( ui_selected_item == (ITEM_EVNT_CONST1+i) && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintSpaces(1);
      PrintCValue(visible_track, i);
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_MIDI_CHANNEL && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString("%3d  ", SEQ_CC_Get(visible_track, SEQ_CC_MIDI_CHANNEL)+1);
  }

  ///////////////////////////////////////////////////////////////////////////
  mios32_midi_port_t port = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_PORT);
  if( ui_selected_item == ITEM_MIDI_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintMIDIOutPort(port);
  }
  ;
  SEQ_LCD_PrintString(SEQ_MIDI_PORT_OutCheckAvailable(port) ? " (available)    " : " (not available)");
  SEQ_LCD_PrintSpaces(5);

  ///////////////////////////////////////////////////////////////////////////

  if( preset_copied ) {
    SEQ_LCD_CursorSet(70, 0);
    SEQ_LCD_PrintString("> PRESET <");
    SEQ_LCD_CursorSet(70, 1);
    SEQ_LCD_PrintString("> COPIED <");
  } else {
    if( old_evnt_mode[visible_track] != SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVNT_MODE) ) {
      SEQ_LCD_CursorSet(70, 0);
      if( !ui_cursor_flash )
	SEQ_LCD_PrintSpaces(10);
      else
	SEQ_LCD_PrintString("!! COPY !!");

      SEQ_LCD_CursorSet(70, 1);
      if( !ui_cursor_flash )
	SEQ_LCD_PrintSpaces(10);
      else
	SEQ_LCD_PrintString("! PRESET !");
    } else {
      SEQ_LCD_CursorSet(70, 0);
      SEQ_LCD_PrintString(">> COPY <<");
      SEQ_LCD_CursorSet(70, 1);
      SEQ_LCD_PrintString("> PRESET <");
    }
  }


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_TRKEVNT_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  preset_copied = 0;

  u8 track;
  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
    old_evnt_mode[track] = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_MODE);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Help function to print a constant value (4 characters)
/////////////////////////////////////////////////////////////////////////////
static s32 PrintCValue(u8 track, u8 const_ix)
{
  u8 value = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_CONST1 + const_ix);

  switch( SEQ_LAYER_GetCControlType(track, const_ix) ) {
    case SEQ_LAYER_ControlType_None:
      SEQ_LCD_PrintSpaces(4);
      break;

    case SEQ_LAYER_ControlType_Note:
      SEQ_LCD_PrintChar(' ');
      SEQ_LCD_PrintNote(value);
      break;

    case SEQ_LAYER_ControlType_Velocity:
    case SEQ_LAYER_ControlType_CC:
      SEQ_LCD_PrintFormattedString(" %3d", value);
      break;

    case SEQ_LAYER_ControlType_Length:
      SEQ_LCD_PrintGatelength(value);
      break;

    case SEQ_LAYER_ControlType_CMEM_T:
    {
      u8 cmem_track = value % SEQ_CORE_NUM_TRACKS;
      SEQ_LCD_PrintFormattedString("G%dT%d", (cmem_track >> 2)+1, (cmem_track & 3)+1);
    }
    break;

    default:
      SEQ_LCD_PrintString(" ???");
      break;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Copies preset
/////////////////////////////////////////////////////////////////////////////
s32 CopyPreset(u8 track)
{
  preset_copied = 1;
  old_evnt_mode[track] = SEQ_CC_Get(track, SEQ_CC_MIDI_EVNT_MODE);
  return SEQ_LAYER_CopyPreset(track, 0); // 0=all settings
}
