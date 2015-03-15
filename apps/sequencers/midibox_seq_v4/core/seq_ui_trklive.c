// $Id: seq_ui_pmute.c 759 2009-10-25 22:10:11Z tk $
/*
 * Live Play Page
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

#include "seq_live.h"
#include "seq_midi_port.h"
#include "seq_midi_in.h"
#include "seq_file.h"
#include "seq_file_c.h"
#include "seq_file_gc.h"

/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#define NUM_OF_ITEMS       12
#define ITEM_GXTY          0
#define ITEM_MUTE          1
#define ITEM_OCT_TRANSPOSE 2
#define ITEM_LIVE_VELOCITY 3
#define ITEM_LIVE_FORCE_SCALE 4
#define ITEM_LIVE_FX       5
#define ITEM_IN_BUS        6
#define ITEM_IN_PORT       7
#define ITEM_IN_CHN        8
#define ITEM_IN_LOWER      9
#define ITEM_IN_UPPER      10
#define ITEM_IN_MODE       11
#define ITEM_RESET_STACKS  12


// future soft-configurable option?
#define GP_BUTTONS_USED_AS_KEYBOARD 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
#if GP_BUTTONS_USED_AS_KEYBOARD
static u16 gp_button_pressed;
#endif

static u8 selected_bus = 0;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
#if GP_BUTTONS_USED_AS_KEYBOARD
  *gp_leds = gp_button_pressed;
#else
  if( ui_cursor_flash ) // if flashing flag active: no LED flag set
    return 0;

  switch( ui_selected_item ) {
    case ITEM_GXTY: *gp_leds = 0x0001; break;
    case ITEM_MUTE: *gp_leds = 0x0002; break;
    case ITEM_OCT_TRANSPOSE: *gp_leds = 0x0004; break;
    case ITEM_LIVE_VELOCITY: *gp_leds = 0x0008; break;
    case ITEM_LIVE_FORCE_SCALE: *gp_leds = 0x0010; break;
    case ITEM_LIVE_FX: *gp_leds = 0x0020; break;
    case ITEM_IN_BUS: *gp_leds |= 0x0100; break;
    case ITEM_IN_PORT: *gp_leds |= 0x0200; break;
    case ITEM_IN_CHN: *gp_leds |= 0x0400; break;
    case ITEM_IN_LOWER: *gp_leds |= 0x0800; break;
    case ITEM_IN_UPPER: *gp_leds |= 0x1000; break;
    case ITEM_IN_MODE: *gp_leds |= 0x2000; break;
    case ITEM_RESET_STACKS: *gp_leds |= 0x8000; break;
  }
#endif

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
  u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);

  // ensure that original record screen will be print immediately
  ui_hold_msg_ctr = 0;

  switch( encoder ) {
    case SEQ_UI_ENCODER_GP1:
      ui_selected_item = ITEM_GXTY;
      break;

    case SEQ_UI_ENCODER_GP2:
      ui_selected_item = ITEM_MUTE;
      break;

    case SEQ_UI_ENCODER_GP3:
      ui_selected_item = ITEM_OCT_TRANSPOSE;
      break;

    case SEQ_UI_ENCODER_GP4:
      ui_selected_item = ITEM_LIVE_VELOCITY;
      break;

    case SEQ_UI_ENCODER_GP5:
      ui_selected_item = ITEM_LIVE_FORCE_SCALE;
      break;

    case SEQ_UI_ENCODER_GP6:
      ui_selected_item = ITEM_LIVE_FX;
      break;

    case SEQ_UI_ENCODER_GP7:
      return 0; // not mapped (yet)

    case SEQ_UI_ENCODER_GP8:
      // enter repeat page
      SEQ_UI_PageSet(SEQ_UI_PAGE_TRKREPEAT);
      return 0;
      break;

    case SEQ_UI_ENCODER_GP9:
      ui_selected_item = ITEM_IN_BUS;
      break;

    case SEQ_UI_ENCODER_GP10:
      ui_selected_item = ITEM_IN_PORT;
      break;

    case SEQ_UI_ENCODER_GP11:
      ui_selected_item = ITEM_IN_CHN;
      break;

    case SEQ_UI_ENCODER_GP12:
      ui_selected_item = ITEM_IN_LOWER;
      break;

    case SEQ_UI_ENCODER_GP13:
      ui_selected_item = ITEM_IN_UPPER;
      break;

    case SEQ_UI_ENCODER_GP14:
      ui_selected_item = ITEM_IN_MODE;
      break;

    case SEQ_UI_ENCODER_GP15:
      return -1; // not mapped

    case SEQ_UI_ENCODER_GP16:
      ui_selected_item = ITEM_RESET_STACKS;
      break;
  }

  // for GP encoders and Datawheel
  switch( ui_selected_item ) {
    case ITEM_GXTY: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;
      return SEQ_UI_GxTyInc(incrementer);
    }

    case ITEM_MUTE: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;

      u16 mask = 1 << visible_track;

      if( incrementer < 0 )
	seq_core_trk_muted |= mask;
      else
	seq_core_trk_muted &= ~mask;
      return 1; // value changed
    } break;

    case ITEM_OCT_TRANSPOSE: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;

      if( event_mode == SEQ_EVENT_MODE_Drum )
	return -1; // disabled

      u8 tmp = seq_live_options.OCT_TRANSPOSE + 5;
      if( SEQ_UI_Var8_Inc(&tmp, 0, 10, incrementer) >= 0 ) {
	seq_live_options.OCT_TRANSPOSE = (s8)tmp - 5;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_LIVE_VELOCITY: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;

      if( SEQ_UI_Var8_Inc(&seq_live_options.VELOCITY, 1, 127, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    }

    case ITEM_LIVE_FORCE_SCALE: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;

      if( incrementer )
	seq_live_options.FORCE_SCALE = incrementer > 0 ? 1 : 0;
      else
	seq_live_options.FORCE_SCALE ^= 1;
      return 1;
    }

    case ITEM_LIVE_FX: {
      if( !seq_midi_in_options[selected_bus].MODE_PLAY )
	return -1;

      if( incrementer )
	seq_live_options.FX = incrementer > 0 ? 1 : 0;
      else
	seq_live_options.FX ^= 1;
      return 1;
    }

    case ITEM_IN_BUS: {
      if( SEQ_UI_Var8_Inc(&selected_bus, 0, SEQ_MIDI_IN_NUM_BUSSES-1, incrementer) >= 0 ) {
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_PORT: {
      u8 port_ix = SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus]);
      if( SEQ_UI_Var8_Inc(&port_ix, 0, SEQ_MIDI_PORT_InNumGet()-1-4, incrementer) >= 0 ) { // don't allow selection of Bus1..Bus4
	seq_midi_in_port[selected_bus] = SEQ_MIDI_PORT_InPortGet(port_ix);
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_IN_CHN:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_channel[selected_bus], 0, 16, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_LOWER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_lower[selected_bus], 0, 127, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_UPPER:
      if( SEQ_UI_Var8_Inc(&seq_midi_in_upper[selected_bus], 0, 127, incrementer) >= 0 ) {
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change

    case ITEM_IN_MODE: {
      u8 fwd = seq_midi_in_options[selected_bus].MODE_PLAY;
      if( SEQ_UI_Var8_Inc(&fwd, 0, 1, incrementer) >= 0 ) {
	seq_midi_in_options[selected_bus].MODE_PLAY = fwd;
	ui_store_file_required = 1;
	return 1; // value changed
      }
      return 0; // no change
    } break;

    case ITEM_RESET_STACKS: {
      SEQ_MIDI_IN_ResetTransArpStacks();
      SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 2000, "Transposer/Arp.", "Stacks cleared!");
      return 1;
    } break;
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
#if GP_BUTTONS_USED_AS_KEYBOARD
  if( button <= SEQ_UI_BUTTON_GP16 ) {
    if( !seq_midi_in_options[selected_bus].MODE_PLAY )
      return -1;

    u8 visible_track = SEQ_UI_VisibleTrackGet();
    mios32_midi_package_t p;

    if( depressed )
      gp_button_pressed &= ~(1 << button);
    else
      gp_button_pressed |= (1 << button);

    p.type = NoteOn;
    p.event = NoteOn;
    p.chn = Chn1; // will be overruled anyhow by SEQ_LIVE_PlayEvent()
    p.note = 0x30 + (u8)button;
    p.velocity = depressed ? 0x00 : seq_live_options.VELOCITY;
    SEQ_LIVE_PlayEvent(visible_track, p);
    return 0;
  }
#endif

  // exit if button depressed
  if( depressed )
    return -1;

  if( button <= SEQ_UI_BUTTON_GP16 ) {
    // -> forward to encoder handler
    return Encoder_Handler((int)button, 0);
  }

  // remaining buttons:
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
  u8 visible_track = SEQ_UI_VisibleTrackGet();

  if( high_prio )
    return 0; // there are no high-priority updates

  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  // Trk. Mute Oct. Vel. FTS   Fx      Repeat Bus Port Chn. Lower/Upper Mode   Reset 
  // G1T1       +0  100   on   on      Config  1  IN1  #16   ---   ---  T&A    Stacks

  // The selected Bus1 is not configured      Bus Port Chn. Lower/Upper Mode   Reset 
  // for Play mode (but for Transposer&Arp.)   1  IN1  #16   ---   ---  T&A    Stacks

  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 0);
  if( !seq_midi_in_options[selected_bus].MODE_PLAY ) {
    SEQ_LCD_PrintFormattedString("The selected Bus%d is not configured     ", selected_bus+1);
  } else {
    SEQ_LCD_PrintString("Trk. Mute Oct. Vel. FTS   Fx      Repeat");
  }

  SEQ_LCD_CursorSet(40, 0);
  SEQ_LCD_PrintString(" Bus Port Chn. Lower/Upper Mode   Reset ");


  ///////////////////////////////////////////////////////////////////////////
  SEQ_LCD_CursorSet(0, 1);
  if( !seq_midi_in_options[selected_bus].MODE_PLAY ) {
    SEQ_LCD_PrintString("for Play mode (but for Transposer&Arp.) ");
  } else {
    if( ui_selected_item == ITEM_GXTY && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    }
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_MUTE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(5);
    } else {
      SEQ_LCD_PrintSpaces(2);
      SEQ_LCD_PrintChar((seq_core_trk_muted & (1 << visible_track)) ? '*' : 'o');
      SEQ_LCD_PrintSpaces(2);
    }

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_OCT_TRANSPOSE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(4);
    } else {
      u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
      if( event_mode == SEQ_EVENT_MODE_Drum ) {
	SEQ_LCD_PrintString("Drum");
      } else {
	SEQ_LCD_PrintFormattedString(" %c%d ", (seq_live_options.OCT_TRANSPOSE < 0) ? '-' : '+', abs(seq_live_options.OCT_TRANSPOSE));
      }
    }
    SEQ_LCD_PrintSpaces(1);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LIVE_VELOCITY && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintFormattedString("%3d", seq_live_options.VELOCITY);
    }
    SEQ_LCD_PrintSpaces(2);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LIVE_FORCE_SCALE && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintString(seq_live_options.FORCE_SCALE ? " on" : "off");
    }
    SEQ_LCD_PrintSpaces(2);

    ///////////////////////////////////////////////////////////////////////////
    if( ui_selected_item == ITEM_LIVE_FX && ui_cursor_flash ) {
      SEQ_LCD_PrintSpaces(3);
    } else {
      SEQ_LCD_PrintString(seq_live_options.FX ? " on" : "off");
    }
    SEQ_LCD_PrintSpaces(2 + 4);

    SEQ_LCD_PrintString("Config");
  }



  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_BUS && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(5);
  } else {
    SEQ_LCD_PrintFormattedString("  %d  ", selected_bus+1);
  }

  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_PORT && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    if( seq_midi_in_port[selected_bus] )
      SEQ_LCD_PrintString(SEQ_MIDI_PORT_InNameGet(SEQ_MIDI_PORT_InIxGet(seq_midi_in_port[selected_bus])));
    else
      SEQ_LCD_PrintString(" All");
  }
  SEQ_LCD_PrintSpaces(1);


  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_CHN && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    if( seq_midi_in_channel[selected_bus] )
      SEQ_LCD_PrintFormattedString("#%2d", seq_midi_in_channel[selected_bus]);
    else
      SEQ_LCD_PrintString("---");
  }
  SEQ_LCD_PrintSpaces(3);


  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_LOWER && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(seq_midi_in_lower[selected_bus]);
  }
  SEQ_LCD_PrintSpaces(3);


  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_UPPER && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(3);
  } else {
    SEQ_LCD_PrintNote(seq_midi_in_upper[selected_bus]);
  }
  SEQ_LCD_PrintSpaces(2);


  ///////////////////////////////////////////////////////////////////////
  if( ui_selected_item == ITEM_IN_MODE && ui_cursor_flash ) {
    SEQ_LCD_PrintSpaces(4);
  } else {
    SEQ_LCD_PrintString(seq_midi_in_options[selected_bus].MODE_PLAY ? "Play" : "T&A ");
  }
  SEQ_LCD_PrintSpaces(3);

  SEQ_LCD_PrintString("Stacks");

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
    MUTEX_SDCARD_TAKE;
    if( (status=SEQ_FILE_C_Write(seq_file_session_name)) < 0 )
      SEQ_UI_SDCardErrMsg(2000, status);
    MUTEX_SDCARD_GIVE;

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
s32 SEQ_UI_TRKLIVE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);
  SEQ_UI_InstallExitCallback(EXIT_Handler);

#if GP_BUTTONS_USED_AS_KEYBOARD
  gp_button_pressed = 0x0000;
#endif

  return 0; // no error
}
