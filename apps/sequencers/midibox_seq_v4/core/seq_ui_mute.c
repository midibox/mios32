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

  if( !ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED ) {
    *gp_leds = latched_mute;
  } else {
    if( seq_ui_button_state.MUTE_PRESSED ) {
      track = SEQ_UI_VisibleTrackGet();
      *gp_leds = seq_core_trk[track].layer_muted;
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
    if( seq_ui_button_state.SELECT_PRESSED ) {
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
      u16 *muted = seq_ui_button_state.MUTE_PRESSED ? (u16 *)&seq_core_trk[visible_track].layer_muted : (u16 *)&seq_core_trk_muted;

      if( incrementer < 0 )
	*muted |= mask;
      else if( incrementer > 0 )
	*muted &= ~mask;
      else
	*muted ^= mask;

      portEXIT_CRITICAL();
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
	  seq_core_trk_muted = latched_mute;
	}
      } else {
	// select pressed: init latched mutes which will be taken over once SELECT button released
	if( seq_ui_button_state.MUTE_PRESSED ) {
	  u8 visible_track = SEQ_UI_VisibleTrackGet();
	  latched_mute = seq_core_trk[visible_track].layer_muted;
	} else {
	  u8 track;
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

  if( high_prio ) {
    ///////////////////////////////////////////////////////////////////////////
    // frequently update VU meters
    SEQ_LCD_CursorSet(0, 1);

    u8 track;
    u16 mute_flags = 0;

    if( !ui_cursor_flash && seq_ui_button_state.SELECT_PRESSED ) {
      mute_flags = latched_mute;
    } else {
      if( seq_ui_button_state.MUTE_PRESSED ) {
	u8 visible_track = SEQ_UI_VisibleTrackGet();
	mute_flags = seq_core_trk[visible_track].layer_muted;
      } else {
	seq_core_trk_t *t = &seq_core_trk[0];
	mute_flags = seq_core_trk_muted;
      }
    }

    if( seq_ui_button_state.MUTE_PRESSED ) {
      u8 layer;
      for(layer=0; layer<16; ++layer)
	if( mute_flags & (1 << layer) )
	  SEQ_LCD_PrintString("Mute ");
	else
	  SEQ_LCD_PrintHBar((seq_layer_vu_meter[layer] >> 3) & 0xf);
    } else {
      seq_core_trk_t *t = &seq_core_trk[0];
      for(track=0; track<16; ++t, ++track)
	if( mute_flags & (1 << track) )
	  SEQ_LCD_PrintString("Mute ");
	else
	  SEQ_LCD_PrintHBar(t->vu_meter >> 3);
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
	    else
	      SEQ_LCD_PrintString(SEQ_PAR_AssignedTypeStr(visible_track, track));
	  }
	} else {
	  // print 'm' if one or more layers are muted
	  SEQ_LCD_PrintChar(seq_core_trk[track].layer_muted ? 'm' : ' ');

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
