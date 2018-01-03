// $Id$
/*
 * Mute page
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
#include "seq_cc.h"
#include "seq_layer.h"
#include "seq_par.h"
#include "seq_bpm.h"
#include "seq_midi_in.h"

#include "seq_core.h"


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u16 latched_mute;


/////////////////////////////////////////////////////////////////////////////
// Local LED handler function
/////////////////////////////////////////////////////////////////////////////
static s32 LED_Handler(u16 *gp_leds)
{
  u8 track;

  if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    *gp_leds = 0;

    u16 all_layers_muted_mask = 0;
    int track;
    for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track)
      all_layers_muted_mask |= seq_core_trk[track].layer_muted;

#if 1
    if( seq_core_trk_muted == 0xffff )
      *gp_leds |= 0x0003;
    if( seq_core_trk[visible_track].layer_muted == 0xffff )
      *gp_leds |= 0x001c;
    if( seq_core_trk_muted == 0xffff && all_layers_muted_mask == 0xffff )
      *gp_leds |= 0x00e0;

    if( seq_core_trk_muted == 0x0000 )
      *gp_leds |= 0x0300;
    if( seq_core_trk[visible_track].layer_muted == 0x0000 )
      *gp_leds |= 0x1c00;
    if( seq_core_trk_muted == 0x0000 && all_layers_muted_mask == 0x0000 )
      *gp_leds |= 0xe000;
#else
    // Better? Mayber to difficult to understand for users, therefore disabled by default
    // e.g. all tracks muted: activate LEDs of the "all tracks unmuted" button.
    // it should signal: something will happen when you push here...
    if( seq_core_trk_muted == 0xffff )
      *gp_leds |= 0x0300;
    if( seq_core_trk[visible_track].layer_muted == 0xffff )
      *gp_leds |= 0x1c00;
    if( seq_core_trk_muted == 0xffff && all_layers_muted_mask == 0xffff )
      *gp_leds |= 0xe000;

    if( seq_core_trk_muted == 0x0000 )
      *gp_leds |= 0x0003;
    if( seq_core_trk[visible_track].layer_muted == 0x0000 )
      *gp_leds |= 0x001c;
    if( seq_core_trk_muted == 0x0000 && all_layers_muted_mask == 0x0000 )
      *gp_leds |= 0x00e0;
#endif

  } else if( !ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED ) {
    *gp_leds = latched_mute;
  } else {
    if( seq_ui_button_state.MUTE_PRESSED ) {
      track = SEQ_UI_VisibleTrackGet();
      *gp_leds = seq_core_trk[track].layer_muted | seq_core_trk[track].layer_muted_from_midi;
    } else {
      *gp_leds = seq_core_trk_muted;
    }
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( (encoder >= SEQ_UI_ENCODER_GP1 && encoder <= SEQ_UI_ENCODER_GP16) ) {
#else
  if( encoder <= SEQ_UI_ENCODER_GP16 ) {
#endif
    if( seq_ui_button_state.SELECT_PRESSED && !seq_ui_button_state.CHANGE_ALL_STEPS ) {
	// select button pressed: indirect MUTED flag modification (taken over when select button depressed)
	u16 mask = 1 << encoder;
	if( incrementer < 0 || (incrementer == 0 && !(latched_mute & mask)) )
	  latched_mute |= mask;
	else
	  latched_mute &= ~mask;
    } else {
      // select button not pressed: direct MUTED flag modification
      // access to seq_core_trk[] must be atomic!
      portENTER_CRITICAL();

      u8 visible_track = SEQ_UI_VisibleTrackGet();
      u16 mask = 1 << encoder;
      u16 *muted = (u16 *)&seq_core_trk_muted;

      if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
	switch( encoder ) {
	case SEQ_UI_ENCODER_GP1:
	case SEQ_UI_ENCODER_GP2:
	  *muted = 0xffff;
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "All Tracks", "muted");
	  break;

	case SEQ_UI_ENCODER_GP3:
	case SEQ_UI_ENCODER_GP4:
	case SEQ_UI_ENCODER_GP5:
	  muted = (u16 *)&seq_core_trk[visible_track].layer_muted;
	  *muted = 0xffff;
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "All Layers", "of current Track muted");
	  break;

	case SEQ_UI_ENCODER_GP6:
	case SEQ_UI_ENCODER_GP7:
	case SEQ_UI_ENCODER_GP8: {
	  *muted = 0xffff;

	  int track;
	  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	    seq_core_trk[track].layer_muted = 0xffff;
	  }
	  SEQ_UI_Msg(SEQ_UI_MSG_USER, 1000, "All Layers", "and Tracks muted");
	} break;

	case SEQ_UI_ENCODER_GP9:
	case SEQ_UI_ENCODER_GP10:
	  *muted = 0x0000;
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "All Tracks", "unmuted");
	  break;

	case SEQ_UI_ENCODER_GP11:
	case SEQ_UI_ENCODER_GP12:
	case SEQ_UI_ENCODER_GP13:
	  muted = (u16 *)&seq_core_trk[visible_track].layer_muted;
	  *muted = 0x0000;
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "All Layers", "of current Track unmuted");
	  break;

	case SEQ_UI_ENCODER_GP14:
	case SEQ_UI_ENCODER_GP15:
	case SEQ_UI_ENCODER_GP16: {
	  *muted = 0x0000;

	  int track;
	  for(track=0; track<SEQ_CORE_NUM_TRACKS; ++track) {
	    seq_core_trk[track].layer_muted = 0x0000;
	  }
	  SEQ_UI_Msg(SEQ_UI_MSG_USER_R, 1000, "All Layers", "and Tracks unmuted");
	} break;
	}
      } else {
	u8 par_layer_mute = 0;
	if( seq_ui_button_state.MUTE_PRESSED ) {
	  par_layer_mute = 1;
	  muted = (u16 *)&seq_core_trk[visible_track].layer_muted;
	} else if( SEQ_BPM_IsRunning() ) { // Synched Mutes only when sequencer is running
	  if( !(*muted & mask) && seq_core_options.SYNCHED_MUTE && !seq_ui_button_state.FAST_ENCODERS ) { // Fast button will disable synched mute
	    muted = (u16 *)&seq_core_trk_synched_mute;
	  } else if( (*muted & mask) && seq_core_options.SYNCHED_UNMUTE && !seq_ui_button_state.FAST_ENCODERS ) { // Fast button will disable synched unmute
	    muted = (u16 *)&seq_core_trk_synched_unmute;
	  }
	} else {
	  // clear synched mutes/unmutes if sequencer not running
	  seq_core_trk_synched_mute = 0;
	  seq_core_trk_synched_unmute = 0;
	}

	if( incrementer < 0 )
	  *muted |= mask;
	else if( incrementer > 0 )
	  *muted &= ~mask;
	else
	  *muted ^= mask;

	if( (*muted & mask) == 0 ) {
	  if( par_layer_mute ) {
	    // simplified usage: select the par layer
	    ui_selected_par_layer = encoder;
	  } else {
	    // simplified usage: select the track
	    ui_selected_tracks = 1 << encoder;
	    ui_selected_group = encoder/4;
	  }
	}
      }

      portEXIT_CRITICAL();

      if( muted == ((u16 *)&seq_core_trk_muted) ) {
	// send to external
	SEQ_MIDI_IN_ExtCtrlSend(SEQ_MIDI_IN_EXT_CTRL_MUTES, (*muted & mask) ? 127 : 0, encoder);
      }
    }

    return 1; // value changed
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
#if 0
  // leads to: comparison is always true due to limited range of data type
  if( button >= SEQ_UI_BUTTON_GP1 && button <= SEQ_UI_BUTTON_GP16 ) {
#else
  if( button <= SEQ_UI_BUTTON_GP16 ) {
#endif
    if( depressed ) return 0; // ignore when button depressed

    // re-using encoder routine
    return Encoder_Handler(button, 0);
  }

  switch( button ) {
    case SEQ_UI_BUTTON_Select:
      portENTER_CRITICAL();
      if( depressed ) {
	// select button released: take over latched mutes
	if( seq_ui_button_state.MUTE_PRESSED ) {
	  u8 visible_track = SEQ_UI_VisibleTrackGet();
	  seq_core_trk[visible_track].layer_muted = latched_mute;
	} else {
	  u16 new_mutes = latched_mute & ~seq_core_trk_muted;
	  if( SEQ_BPM_IsRunning() && seq_core_options.SYNCHED_MUTE && !seq_ui_button_state.FAST_ENCODERS ) // Fast button will disable synched mute
	    seq_core_trk_synched_mute |= new_mutes;
	  else
	    seq_core_trk_muted |= new_mutes;

	  u16 new_unmutes = ~latched_mute & seq_core_trk_muted;
	  if( SEQ_BPM_IsRunning() && seq_core_options.SYNCHED_UNMUTE && !seq_ui_button_state.FAST_ENCODERS ) // Fast button will disable synched unmute
	    seq_core_trk_synched_unmute |= new_unmutes;
	  else
	    seq_core_trk_muted &= ~new_unmutes;
	}
      } else {
	// select pressed: init latched mutes which will be taken over once SELECT button released
	if( seq_ui_button_state.MUTE_PRESSED ) {
	  u8 visible_track = SEQ_UI_VisibleTrackGet();
	  latched_mute = seq_core_trk[visible_track].layer_muted;
	} else {
	  latched_mute = seq_core_trk_muted;
	}
      }

      portEXIT_CRITICAL();
      return 1;
  }

  return -1; // invalid or unsupported button
}


/////////////////////////////////////////////////////////////////////////////
// Local Display Handler function
// IN: <high_prio>: if set, a high-priority LCD update is requested
/////////////////////////////////////////////////////////////////////////////
static s32 LCD_Handler(u8 high_prio)
{
  // layout:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //  > 1<   2    3    4    5    6    7    8    9   10   11   12   13   14   15   16 
  // ...horizontal VU meters...

  // Mute All screen:
  // 00000000001111111111222222222233333333330000000000111111111122222222223333333333
  // 01234567890123456789012345678901234567890123456789012345678901234567890123456789
  // <--------------------------------------><-------------------------------------->
  //    Mute       Mute       Mute all Tracks  Unmute      Unmute   Unmute all Tracks
  // all Tracks G1T1 Layers    and all Layersall Tracks G1T1 Layers    and all Layers

  if( seq_ui_button_state.CHANGE_ALL_STEPS ) {
    if( high_prio )
      return 0;

    SEQ_LCD_CursorSet(0, 0);
    SEQ_LCD_PrintString("   Mute       Mute       Mute all Tracks  Unmute      Unmute   Unmute all Tracks");
    SEQ_LCD_CursorSet(0, 1);
    SEQ_LCD_PrintString("all Tracks ");
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintString(" Layers    and all Layersall Tracks ");
    SEQ_LCD_PrintGxTy(ui_selected_group, ui_selected_tracks);
    SEQ_LCD_PrintString(" Layers    and all Layers");

    return 0;
  }

  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters
    SEQ_LCD_CursorSet(0, 1);

    u8 track;
    u16 mute_flags = 0;
    u16 mute_flags_from_midi = 0;

    if( !ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED ) {
      mute_flags = latched_mute;
    } else {
      if( seq_ui_button_state.MUTE_PRESSED ) {
	u8 visible_track = SEQ_UI_VisibleTrackGet();
	mute_flags = seq_core_trk[visible_track].layer_muted;
	mute_flags_from_midi = seq_core_trk[visible_track].layer_muted_from_midi;
      } else {
	mute_flags = seq_core_trk_muted;
      }
    }

    if( seq_ui_button_state.MUTE_PRESSED ) {
      u8 layer;
      u16 mask = (1 << 0);
      for(layer=0; layer<16; ++layer, mask <<= 1)
	if( mute_flags_from_midi & mask ) {
	  SEQ_LCD_PrintString("MIDI ");
	} else if( mute_flags & mask ) {
	  SEQ_LCD_PrintString("Mute ");
	} else {
	  SEQ_LCD_PrintHBar((seq_layer_vu_meter[layer] >> 3) & 0xf);
	}
    } else {
      int remaining_steps = (seq_core_steps_per_measure - seq_core_state.ref_step) + 1;
      seq_core_trk_t *t = &seq_core_trk[0];
      u16 mask = (1 << 0);
      for(track=0; track<16; ++t, ++track, mask <<= 1)
	if( mute_flags & mask ) {
	  if( !seq_ui_button_state.SELECT_PRESSED && (seq_core_trk_synched_unmute & mask) ) {
	    SEQ_LCD_PrintFormattedString("U%3d ", remaining_steps);
	  } else {
	    SEQ_LCD_PrintString("Mute ");
	  }
	} else {
	  if( !seq_ui_button_state.SELECT_PRESSED && (seq_core_trk_synched_mute & mask) ) {
	    SEQ_LCD_PrintFormattedString("M%3d ", remaining_steps);
	  } else {
	    SEQ_LCD_PrintHBar(t->vu_meter >> 3);
	  }
	}
    }
  } else {
    ///////////////////////////////////////////////////////////////////////////
    SEQ_LCD_CursorSet(0, 0);

    u8 track;
    u8 visible_track = SEQ_UI_VisibleTrackGet();
    u8 event_mode = SEQ_CC_Get(visible_track, SEQ_CC_MIDI_EVENT_MODE);
    u8 num_layers = (event_mode == SEQ_EVENT_MODE_Drum) ? SEQ_TRG_NumInstrumentsGet(visible_track) : SEQ_PAR_NumLayersGet(visible_track);

    for(track=0; track<16; ++track) {
      if( ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED )
	SEQ_LCD_PrintSpaces(5);
      else {
	if( seq_ui_button_state.MUTE_PRESSED ) {
	  if( track >= num_layers )
	    SEQ_LCD_PrintSpaces(5);
	  else {
	    if( event_mode == SEQ_EVENT_MODE_Drum )
	      SEQ_LCD_PrintTrackDrum(visible_track, track, (char *)seq_core_trk[visible_track].name);
	    else {
	      char str_buffer[6];
	      SEQ_PAR_AssignedTypeStr(visible_track, track, ui_selected_instrument, str_buffer);
	      SEQ_LCD_PrintString(str_buffer);
	    }
	  }
	} else {
	  // print 'm' if one or more layers are muted
	  SEQ_LCD_PrintChar(seq_core_trk[track].layer_muted ? 'm' : (seq_core_trk[track].layer_muted_from_midi ? 'M' : ' '));

	  if( SEQ_UI_IsSelectedTrack(track) )
	    SEQ_LCD_PrintFormattedString(">%2d<", track+1);
	  else
	    SEQ_LCD_PrintFormattedString(" %2d ", track+1);
	}
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Initialisation
/////////////////////////////////////////////////////////////////////////////
s32 SEQ_UI_MUTE_Init(u32 mode)
{
  // install callback routines
  SEQ_UI_InstallButtonCallback(Button_Handler);
  SEQ_UI_InstallEncoderCallback(Encoder_Handler);
  SEQ_UI_InstallLEDCallback(LED_Handler);
  SEQ_UI_InstallLCDCallback(LCD_Handler);

  // we want to show horizontal VU meters
  SEQ_LCD_InitSpecialChars(SEQ_LCD_CHARSET_HBars);

  latched_mute = 0;

  return 0; // no error
}
